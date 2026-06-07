#include "server.h"

#include "session.h"
#include "echo_message_processor.h"

#include <spdlog/spdlog.h>

Server::Server(asio::io_context& io_context, unsigned short port)
    : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
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
                std::make_shared<Session>(std::move(socket), std::make_shared<EchoMessageProcessor>())->start();
            } else {
                spdlog::error("Accept error: {}", ec.message());
            }

            start_accept();
        });
}