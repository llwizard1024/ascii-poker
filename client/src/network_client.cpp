#include "network_client.h"

#include <asio/buffer.hpp>
#include <asio/connect.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

#include <json/json.hpp>
#include <spdlog/spdlog.h>

#include <variant>

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
    auto self = shared_from_this();
    asio::async_read(
        socket_,
        asio::buffer(header_buffer_, 4),
        asio::transfer_exactly(4),
        [this, self](const std::error_code& ec, std::size_t bytes_transferred) {
            if (!ec && bytes_transferred == 4) {
                uint32_t network_length;
                std::memcpy(&network_length, header_buffer_.data(), sizeof(network_length));
                uint32_t length = ntohl(network_length);
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
    auto self = shared_from_this();
    asio::async_read(
        socket_,
        asio::buffer(body_buffer_, length),
        [this, self](const std::error_code& ec, std::size_t) {
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

    std::visit([](const auto& concrete_msg) {
        using T = std::decay_t<decltype(concrete_msg)>;
        if constexpr (std::is_same_v<T, pp::RoomList>) {
            spdlog::info("Room List Message");
        } else if constexpr (std::is_same_v<T, pp::JoinedRoom>) {
            spdlog::info("Joined Room Message");
        } else if constexpr (std::is_same_v<T, pp::GameStateUpdate>) {
            spdlog::info("Game State Update Message");
        } else if constexpr (std::is_same_v<T, pp::YourTurn>) {
            spdlog::info("Your Turn Message");
        } else if constexpr (std::is_same_v<T, pp::Error>) {
            spdlog::info("Error Message");
        }
    },
        msg);

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
    std::string json_str = nlohmann::json(msg).dump();
    uint32_t length = static_cast<uint32_t>(json_str.size());

    std::vector<uint8_t> packet;
    packet.reserve(sizeof(length) + length);

    uint32_t network_length = htonl(length);

    const uint8_t* length_bytes = reinterpret_cast<const uint8_t*>(&network_length);

    packet.insert(packet.end(), length_bytes, length_bytes + sizeof(network_length));
    packet.insert(packet.end(), json_str.begin(), json_str.end());

    bool write_in_progress = !write_queue_.empty();

    write_queue_.push_back(std::move(packet));

    if (!write_in_progress) {
        do_write();
    }
}

void NetworkClient::do_write()
{
    auto self = shared_from_this();

    asio::async_write(
        socket_,
        asio::buffer(write_queue_.front().data(), write_queue_.front().size()),
        [this, self](const std::error_code& ec, std::size_t bytes_transferred) {
            if (!ec) {
                spdlog::debug("Response successfully sent. Bytes: {}", bytes_transferred);

                write_queue_.pop_front();

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
    if (!socket_.is_open()) {
        return;
    }

    std::error_code ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);

    socket_.close(ec);

    write_queue_.clear();

    spdlog::info("Connection successfully closed and cleaned up.");
}