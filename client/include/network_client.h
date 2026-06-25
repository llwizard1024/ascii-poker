#pragma once

#include <array>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "poker/protocol.h"

namespace poker::client {

class NetworkClient : public std::enable_shared_from_this<NetworkClient> {
public:
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Failed
    };

    using MessageHandler = std::function<void(const poker::protocol::ServerMessage&)>;
    using ConnectionHandler = std::function<void(ConnectionState)>;

    NetworkClient(asio::io_context& io, std::string host, std::string port);
    ~NetworkClient();

    void start();
    void reconnect();
    void set_endpoint(std::string host, std::string port);
    void send(const poker::protocol::ClientMessage& msg);
    void set_message_handler(MessageHandler handler);
    void set_connection_handler(ConnectionHandler handler);

    ConnectionState connection_state() const { return connection_state_; }
    bool is_connected() const { return connection_state_ == ConnectionState::Connected; }

private:
    static constexpr std::chrono::seconds kConnectTimeout { 5 };

    void begin_connect();
    void on_connect_result(const std::error_code& ec);
    void arm_connect_timeout();
    void cancel_connect_timeout();
    void do_read_header();
    void do_read_body(uint32_t length);
    void on_message(const std::string& json);
    void do_write();
    void close_connection(bool notify);

    asio::io_context& io_;
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::resolver resolver_;
    asio::steady_timer connect_timer_;
    std::string host_;
    std::string port_;
    std::array<uint8_t, 4> header_buffer_;
    std::string body_buffer_;
    std::deque<std::vector<uint8_t>> write_queue_;
    MessageHandler message_handler_;
    ConnectionHandler connection_handler_;
    ConnectionState connection_state_ = ConnectionState::Disconnected;
    bool closed_ = true;
    bool write_in_progress_ = false;
};

} // namespace poker::client
