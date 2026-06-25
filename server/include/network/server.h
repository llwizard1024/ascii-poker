#pragma once

#include "asio/ip/tcp.hpp"
#include "game/lobby.h"
#include "network/connection_registry.h"
#include "network/player_name_registry.h"
#include "storage/database.h"
#include "storage/user_repository.h"

#include <memory>
#include <string>

namespace poker::server {

class Server {
public:
    Server(asio::io_context& io_context, unsigned short port, std::string database_path);
    void start_accept();

    Database& database() { return *database_; }
    UserRepository& users() { return *users_; }

private:
    asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<ConnectionRegistry> connection_registry_;
    std::unique_ptr<Database> database_;
    std::unique_ptr<UserRepository> users_;
    std::shared_ptr<Lobby> lobby_;
    std::shared_ptr<PlayerNameRegistry> name_registry_;
};

} // namespace poker::server
