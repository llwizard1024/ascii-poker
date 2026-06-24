#include "application.h"
#include "client_settings.h"
#include "network_client.h"
#include "poker_ui.h"

#include <asio/io_context.hpp>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>

namespace {
std::string parse_host(int argc, char* argv[], const std::string& default_host)
{
    return argc >= 2 ? argv[1] : default_host;
}

std::string parse_port(int argc, char* argv[], const std::string& default_port)
{
    return argc >= 3 ? argv[2] : default_port;
}
}

int main(int argc, char* argv[])
{
    spdlog::set_level(spdlog::level::info);

    try {
        poker::client::ClientSettings settings = poker::client::load_client_settings();
        const std::string host = parse_host(argc, argv, settings.host);
        const std::string port = parse_port(argc, argv, settings.port);

        asio::io_context io;
        auto client = std::make_shared<poker::client::NetworkClient>(io, host, port);
        auto app = std::make_shared<poker::client::ClientApplication>(client);
        auto ui = std::make_shared<poker::client::PokerUI>(app);

        ui->initialize_session(settings, host, port);

        client->set_message_handler([ui](const poker::protocol::ServerMessage& msg) {
            ui->add_server_message(msg);
        });

        client->set_connection_handler([ui](poker::client::NetworkClient::ConnectionState state) {
            poker::client::ConnectionStatus status = poker::client::ConnectionStatus::Disconnected;
            switch (state) {
            case poker::client::NetworkClient::ConnectionState::Connecting:
                status = poker::client::ConnectionStatus::Connecting;
                break;
            case poker::client::NetworkClient::ConnectionState::Connected:
                status = poker::client::ConnectionStatus::Connected;
                break;
            case poker::client::NetworkClient::ConnectionState::Failed:
                status = poker::client::ConnectionStatus::Failed;
                break;
            case poker::client::NetworkClient::ConnectionState::Disconnected:
                status = poker::client::ConnectionStatus::Disconnected;
                break;
            }
            ui->on_connection_changed(status);
        });

        client->start();

        std::thread io_thread([&io]() { io.run(); });

        ui->run();

        io.stop();
        io_thread.join();
    } catch (const std::exception& e) {
        spdlog::critical("Fatal: {}", e.what());
        return 1;
    }
    return 0;
}
