#pragma once

#include "../game/lobby.h"
#include "asio/ip/tcp.hpp"

#include <memory>

class Server {
public:
    Server(asio::io_context& io_context, unsigned short port);
    void start_accept();

private:
    asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<Lobby> lobby_;
};