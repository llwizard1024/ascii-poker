#include "application.h"
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
        const std::string host = parse_host(argc, argv, "127.0.0.1");
        const std::string port = parse_port(argc, argv, "12345");

        asio::io_context io;
        auto client = std::make_shared<poker::client::NetworkClient>(io, host, port);
        auto app = std::make_shared<poker::client::ClientApplication>(client);
        auto ui = std::make_shared<poker::client::PokerUI>(app);

        client->set_message_handler([ui](const poker::protocol::ServerMessage& msg) {
            ui->add_server_message(msg);
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
