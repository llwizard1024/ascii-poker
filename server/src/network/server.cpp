#include "network/server.h"

#include "messaging/handshake_message_processor.h"
#include "network/connection_registry.h"
#include "network/session.h"
#include "storage/database.h"

#include <spdlog/spdlog.h>

namespace poker::server {

Server::Server(asio::io_context& io_context, unsigned short port, std::string database_path)
    : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
    , connection_registry_(std::make_shared<ConnectionRegistry>())
    , database_(std::make_unique<Database>(std::move(database_path)))
    , users_(std::make_unique<UserRepository>(*database_))
    , lobby_(std::make_shared<Lobby>(connection_registry_, users_.get()))
    , name_registry_(std::make_shared<PlayerNameRegistry>())
{
    spdlog::info("Server created on port {}", port);
}

void Server::start_accept()
{
    acceptor_.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                spdlog::info("New client connected from {}",
                    socket.remote_endpoint().address().to_string());
                auto session = std::make_shared<Session>(
                    std::move(socket),
                    std::make_shared<HandshakeMessageProcessor>(
                        lobby_, name_registry_, connection_registry_, users_.get()));
                session->start();
            } else {
                spdlog::error("Accept error: {}", ec.message());
            }

            start_accept();
        });
}

} // namespace poker::server
