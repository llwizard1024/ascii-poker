#include "network_client.h"

#include <asio/buffer.hpp>
#include <asio/connect.hpp>
#include <asio/post.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

#include <json/json.hpp>
#include <poker/packet_framing.h>
#include <spdlog/spdlog.h>

#include <variant>

namespace poker::client {

namespace pp = poker::protocol;

NetworkClient::NetworkClient(asio::io_context& io, std::string host, std::string port)
    : io_(io)
    , socket_(io)
    , resolver_(io)
    , connect_timer_(io)
    , host_(std::move(host))
    , port_(std::move(port))
{
}

NetworkClient::~NetworkClient()
{
    close_connection(false);
}

void NetworkClient::set_message_handler(MessageHandler handler)
{
    message_handler_ = std::move(handler);
}

void NetworkClient::set_connection_handler(ConnectionHandler handler)
{
    connection_handler_ = std::move(handler);
}

void NetworkClient::start()
{
    begin_connect();
}

void NetworkClient::reconnect()
{
    close_connection(false);
    begin_connect();
}

void NetworkClient::begin_connect()
{
    if (connection_state_ == ConnectionState::Connecting) {
        return;
    }

    std::error_code ec;
    if (socket_.is_open()) {
        socket_.close(ec);
    }

    socket_ = asio::ip::tcp::socket(io_);
    closed_ = false;
    connection_state_ = ConnectionState::Connecting;
    if (connection_handler_) {
        connection_handler_(connection_state_);
    }

    auto self = shared_from_this();
    resolver_.async_resolve(
        host_,
        port_,
        [this, self](const std::error_code& resolve_ec, asio::ip::tcp::resolver::results_type endpoints) {
            if (closed_) {
                return;
            }

            if (resolve_ec) {
                on_connect_result(resolve_ec);
                return;
            }

            arm_connect_timeout();
            asio::async_connect(
                socket_,
                endpoints,
                [this, self](const std::error_code& connect_ec, const asio::ip::tcp::endpoint&) {
                    cancel_connect_timeout();
                    on_connect_result(connect_ec);
                });
        });
}

void NetworkClient::arm_connect_timeout()
{
    connect_timer_.expires_after(kConnectTimeout);
    auto self = shared_from_this();
    connect_timer_.async_wait([this, self](const std::error_code& ec) {
        if (ec || closed_ || connection_state_ != ConnectionState::Connecting) {
            return;
        }

        spdlog::warn("Connection timed out");
        std::error_code close_ec;
        socket_.close(close_ec);
        on_connect_result(asio::error::timed_out);
    });
}

void NetworkClient::cancel_connect_timeout()
{
    connect_timer_.cancel();
}

void NetworkClient::on_connect_result(const std::error_code& ec)
{
    if (closed_) {
        return;
    }

    if (!ec) {
        connection_state_ = ConnectionState::Connected;
        spdlog::info("Connected to {}:{}", host_, port_);
        if (connection_handler_) {
            connection_handler_(connection_state_);
        }
        if (!write_queue_.empty() && !write_in_progress_) {
            do_write();
        }
        do_read_header();
        return;
    }

    if (ec == asio::error::operation_aborted) {
        return;
    }

    closed_ = true;
    connection_state_ = ConnectionState::Failed;
    write_queue_.clear();
    spdlog::error("Connection failed: {}", ec.message());
    if (connection_handler_) {
        connection_handler_(connection_state_);
    }
}

void NetworkClient::do_read_header()
{
    if (closed_ || connection_state_ != ConnectionState::Connected) {
        return;
    }

    auto self = shared_from_this();
    asio::async_read(
        socket_,
        asio::buffer(header_buffer_, 4),
        asio::transfer_exactly(4),
        [this, self](const std::error_code& ec, std::size_t bytes_transferred) {
            if (closed_) {
                return;
            }

            if (!ec && bytes_transferred == 4) {
                uint32_t length = 0;
                if (!poker::network::decode_packet_length(header_buffer_.data(), length)) {
                    spdlog::warn("Invalid packet size. Closing connection.");
                    close_connection(true);
                    return;
                }
                body_buffer_.resize(length);
                do_read_body(length);
            } else {
                spdlog::warn("Read error: {}. Closing session.", ec.message());
                close_connection(true);
            }
        });
}

void NetworkClient::do_read_body(uint32_t length)
{
    if (closed_) {
        return;
    }

    auto self = shared_from_this();
    asio::async_read(
        socket_,
        asio::buffer(body_buffer_, length),
        asio::transfer_exactly(length),
        [this, self](const std::error_code& ec, std::size_t) {
            if (closed_) {
                return;
            }

            if (!ec) {
                on_message(body_buffer_);
                do_read_header();
            } else {
                close_connection(true);
            }
        });
}

void NetworkClient::on_message(const std::string& json)
{
    nlohmann::json data;

    try {
        data = nlohmann::json::parse(json);
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error(e.what());
        return;
    }

    pp::ServerMessage msg;

    try {
        msg = data.get<pp::ServerMessage>();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        return;
    }

    try {
        if (message_handler_) {
            message_handler_(msg);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error in message handler: {}", e.what());
    }
}

void NetworkClient::send(const pp::ClientMessage& msg)
{
    const std::string json_str = nlohmann::json(msg).dump();
    if (json_str.size() > poker::network::MAX_PACKET_SIZE) {
        spdlog::error("Message too large to send");
        return;
    }

    auto packet = poker::network::encode_packet(json_str);

    auto self = shared_from_this();
    asio::post(socket_.get_executor(), [this, self, packet = std::move(packet)]() mutable {
        if (connection_state_ == ConnectionState::Connecting) {
            write_queue_.push_back(std::move(packet));
            return;
        }

        if (!is_connected()) {
            return;
        }

        write_queue_.push_back(std::move(packet));
        if (!write_in_progress_) {
            do_write();
        }
    });
}

void NetworkClient::do_write()
{
    if (!is_connected() || write_in_progress_ || write_queue_.empty()) {
        return;
    }

    write_in_progress_ = true;
    auto self = shared_from_this();
    auto packet = std::make_shared<std::vector<uint8_t>>(write_queue_.front());

    asio::async_write(
        socket_,
        asio::buffer(*packet),
        [this, self, packet](const std::error_code& ec, std::size_t bytes_transferred) {
            write_in_progress_ = false;

            if (!is_connected()) {
                return;
            }

            if (!ec) {
                spdlog::debug("Response successfully sent. Bytes: {}", bytes_transferred);

                if (!write_queue_.empty()) {
                    write_queue_.pop_front();
                }

                if (!write_queue_.empty()) {
                    do_write();
                }
            } else {
                spdlog::error("Write error: {}. Closing connection.", ec.message());
                close_connection(true);
            }
        });
}

void NetworkClient::close_connection(bool notify)
{
    cancel_connect_timeout();

    const bool was_connected = connection_state_ == ConnectionState::Connected
        || connection_state_ == ConnectionState::Connecting;

    closed_ = true;
    write_queue_.clear();
    write_in_progress_ = false;

    std::error_code ec;
    if (socket_.is_open()) {
        socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }

    if (notify && was_connected) {
        connection_state_ = ConnectionState::Disconnected;
        spdlog::info("Connection closed.");
        if (connection_handler_) {
            connection_handler_(connection_state_);
        }
    } else if (connection_state_ == ConnectionState::Connecting) {
        connection_state_ = ConnectionState::Disconnected;
    }
}

} // namespace poker::client
