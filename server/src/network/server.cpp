#include "server.h"

#include "../messaging/lobby_message_processor.h"
#include "session.h"

#include <spdlog/spdlog.h>

Server::Server(asio::io_context& io_context, unsigned short port)
    : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
    , lobby_(std::make_shared<Lobby>())
{
    spdlog::info("Server created on port {}", port);
}

void Server::start_accept()
{
    static int id_counter = 0;
    acceptor_.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                spdlog::info("New client connected from {}",
                    socket.remote_endpoint().address().to_string());
                std::make_shared<Session>(std::move(socket), std::make_shared<LobbyMessageProcessor>(lobby_), "Player " + std::to_string(++id_counter))->start();
            } else {
                spdlog::error("Accept error: {}", ec.message());
            }

            start_accept();
        });
}