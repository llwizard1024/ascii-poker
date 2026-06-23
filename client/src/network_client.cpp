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

NetworkClient::NetworkClient(asio::io_context& io, const std::string& host, const std::string& port)
    : socket_(io)
{
    asio::ip::tcp::resolver resolver(io);
    spdlog::info("Resolving... {} {}", host, port);

    std::error_code ec;

    asio::ip::basic_resolver_results<asio::ip::tcp> endpoints = resolver.resolve(host, port, ec);
    if (ec) {
        spdlog::error("Resolution error: {}", ec.message());
        throw std::runtime_error("Failed to resolve host");
    }

    spdlog::info("Connecting to endpoints...");

    asio::connect(socket_, endpoints, ec);
    if (ec) {
        spdlog::error("Connection error: {}", ec.message());
        throw std::runtime_error("Failed to connect to server");
    }

    spdlog::info("Successfully connected to {}", host);
}

NetworkClient::~NetworkClient()
{
    closed_ = true;
    if (socket_.is_open()) {
        std::error_code ec;
        socket_.close(ec);
    }
}

void NetworkClient::set_message_handler(MessageHandler handler)
{
    handler_ = std::move(handler);
}

void NetworkClient::start()
{
    do_read_header();
}

void NetworkClient::do_read_header()
{
    if (closed_) {
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
                    close_connection();
                    return;
                }
                body_buffer_.resize(length);
                do_read_body(length);
            } else {
                spdlog::warn("Read error: {}. Closing session.", ec.message());
                close_connection();
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
                close_connection();
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
        if (handler_) {
            handler_(msg);
        }
    } catch (const std::exception& e) {
        spdlog::error("Error in message handler: {}", e.what());
    }
}

void NetworkClient::send(const pp::ClientMessage& msg)
{
    if (closed_) {
        return;
    }

    const std::string json_str = nlohmann::json(msg).dump();
    if (json_str.size() > poker::network::MAX_PACKET_SIZE) {
        spdlog::error("Message too large to send");
        return;
    }

    auto packet = poker::network::encode_packet(json_str);

    auto self = shared_from_this();
    asio::post(socket_.get_executor(), [this, self, packet = std::move(packet)]() mutable {
        if (closed_) {
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
    if (closed_ || write_in_progress_ || write_queue_.empty()) {
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

            if (closed_) {
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
                close_connection();
            }
        });
}

void NetworkClient::close_connection()
{
    if (closed_) {
        return;
    }

    closed_ = true;

    std::error_code ec;
    if (socket_.is_open()) {
        socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }

    spdlog::info("Connection successfully closed and cleaned up.");
}

} // namespace poker::client
