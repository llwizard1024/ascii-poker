#pragma once

#include "asio/ip/tcp.hpp"
#include "game/lobby.h"
#include "network/connection_registry.h"
#include "network/player_name_registry.h"

#include <memory>

namespace poker::server {

class Server {
public:
    Server(asio::io_context& io_context, unsigned short port);
    void start_accept();

private:
    asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<ConnectionRegistry> connection_registry_;
    std::shared_ptr<Lobby> lobby_;
    std::shared_ptr<PlayerNameRegistry> name_registry_;
};

} // namespace poker::server
