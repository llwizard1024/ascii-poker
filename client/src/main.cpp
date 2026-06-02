#include "network_client.h"

#include "poker/protocol.h"

#include <asio/io_context.hpp>
#include <spdlog/spdlog.h>

#include <iostream>

int main() 
{
    spdlog::set_level(spdlog::level::info);

    try {
        asio::io_context io;
        auto client = std::make_shared<NetworkClient>(io, "127.0.0.1", "12345");

        client->set_message_handler([](const poker::protocol::ServerMessage& msg) {
            spdlog::info("Server response: {}", nlohmann::json(msg).dump());
        });

        client->start();
        client->send(poker::protocol::ClientMessage{
            poker::protocol::CreateRoom{"test_room", 4}
        });

        io.run();
    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    }

    return 0;
}