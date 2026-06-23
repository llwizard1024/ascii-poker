#include "network_client.h"
#include "application.h"
#include "poker_ui.h"
#include "poker/protocol.h"

#include <asio/io_context.hpp>
#include <spdlog/spdlog.h>
#include <thread>
#include <atomic>

int main() {
    spdlog::set_level(spdlog::level::info);

    try {
        asio::io_context io;
        auto client = std::make_shared<NetworkClient>(io, "127.0.0.1", "12345");
        auto app = std::make_shared<ClientApplication>(client);
        auto ui = std::make_shared<PokerUI>(app);

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