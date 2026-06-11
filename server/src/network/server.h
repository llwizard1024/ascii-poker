#pragma once

#include "asio/ip/tcp.hpp"
#include <memory>

class Server {
public:
    Server(asio::io_context& io_context, unsigned short port);
    void start_accept();

private:
    asio::ip::tcp::acceptor acceptor_;
};