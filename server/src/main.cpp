#include "server.h"

#include <asio/io_context.hpp>
#include <csignal>
#include <spdlog/spdlog.h>

namespace {
asio::io_context* g_io_context = nullptr;
}

void signal_handler(int)
{
    if (g_io_context) {
        g_io_context->stop();
    }
}

int main()
{
    spdlog::set_level(spdlog::level::debug);

    try {
        asio::io_context io_context;
        g_io_context = &io_context;

        Server server(io_context, 12345);
        server.start_accept();

        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        spdlog::info("Server running on port 12345... Press Ctrl+C to stop.");
        io_context.run();
        spdlog::info("Server stopped.");
    } catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}", e.what());
        return 1;
    }

    return 0;
}