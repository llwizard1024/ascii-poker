#pragma once

#include <array>
#include <asio/ip/tcp.hpp>
#include <memory>
#include <string>

#include "poker/protocol.h"

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(asio::ip::tcp::socket socket);
    ~Session();
    void start();

private:
    void do_read_header();
    void do_read_body(uint32_t length);
    void on_message(const std::string& json);
    void send_response(const poker::protocol::ServerMessage& msg);

    asio::ip::tcp::socket socket_;
    std::array<uint8_t, 4> header_buffer_;
    std::string body_buffer_;
};