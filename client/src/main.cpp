#include "network_client.h"

#include "application.h"
#include "poker/protocol.h"

#include <asio/io_context.hpp>
#include <spdlog/spdlog.h>

#include <iostream>
#include <string>
#include <thread>

int main()
{
    spdlog::set_level(spdlog::level::info);

    try {
        asio::io_context io;
        auto client = std::make_shared<NetworkClient>(io, "127.0.0.1", "12345");

        ClientApplication app(client);
        client->set_message_handler([](const poker::protocol::ServerMessage& msg) {
            spdlog::info("Server response: {}", nlohmann::json(msg).dump());
        });

        client->start();

        std::thread input_thread([&]() {
            std::string line;
            while (std::getline(std::cin, line)) {
                app.process_input(line);
                if (app.quit_requested()) {
                    io.stop();
                    break;
                }
            }
        });

        io.run();

        input_thread.join();
    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    }

    return 0;
}