#pragma once

#include <array>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "poker/protocol.h"

class NetworkClient : public std::enable_shared_from_this<NetworkClient> {
public:
    using MessageHandler = std::function<void(const poker::protocol::ServerMessage&)>;

    NetworkClient(asio::io_context& io, const std::string& host, const std::string& port);
    ~NetworkClient();

    void start();
    void send(const poker::protocol::ClientMessage& msg);
    void set_message_handler(MessageHandler handler);

private:
    void do_read_header();
    void do_read_body(uint32_t length);
    void on_message(const std::string& json);
    void do_write();
    void close_connection();

    asio::ip::tcp::socket socket_;
    std::array<uint8_t, 4> header_buffer_;
    std::string body_buffer_;
    std::deque<std::vector<uint8_t>> write_queue_;
    MessageHandler handler_;
};