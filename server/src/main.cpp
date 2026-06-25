#include "network/server.h"

#include <asio/io_context.hpp>
#include <asio/signal_set.hpp>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <storage/database.h>
#include <string>

namespace {
unsigned short parse_port(int argc, char* argv[], unsigned short default_port)
{
    if (argc < 2) {
        return default_port;
    }

    const int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        throw std::invalid_argument("Port must be between 1 and 65535");
    }

    return static_cast<unsigned short>(port);
}
}

int main(int argc, char* argv[])
{
    spdlog::set_level(spdlog::level::debug);

    try {
        const unsigned short port = parse_port(argc, argv, 12345);
        const std::string database_path = poker::server::default_database_path();
        asio::io_context io_context;
        poker::server::Server server(io_context, port, database_path);
        server.start_accept();

        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context](const std::error_code&, int) {
            io_context.stop();
        });

        spdlog::info("Server running on port {}... Press Ctrl+C to stop.", port);
        io_context.run();
        spdlog::info("Server stopped.");
    } catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}", e.what());
        return 1;
    }

    return 0;
}
