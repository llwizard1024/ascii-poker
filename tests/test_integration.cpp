#include "message_collector.h"
#include "network/server.h"

#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <catch_amalgamated.hpp>
#include <poker/protocol.h>

#include <chrono>
#include <memory>
#include <thread>

namespace pp = poker::protocol;

using namespace poker::client;
using poker::test::MessageCollector;

TEST_CASE("Two TCP clients can create a room and start a game", "[integration]")
{
    constexpr unsigned short kPort = 19987;

    asio::io_context server_io;
    auto server_guard = asio::make_work_guard(server_io);
    poker::server::Server server(server_io, kPort);
    server.start_accept();

    std::thread server_thread([&]() { server_io.run(); });

    asio::io_context client_io;
    auto client_guard = asio::make_work_guard(client_io);

    MessageCollector alice_msgs;
    MessageCollector bob_msgs;

    const std::string port_str = std::to_string(kPort);
    auto alice = std::make_shared<NetworkClient>(client_io, "127.0.0.1", port_str);
    auto bob = std::make_shared<NetworkClient>(client_io, "127.0.0.1", port_str);

    alice_msgs.attach(alice);
    bob_msgs.attach(bob);

    alice->start();
    bob->start();

    std::thread client_thread([&]() { client_io.run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    alice->send(pp::ClientMessage { pp::Hello { "Alice" } });
    REQUIRE(alice_msgs.wait_for<pp::Welcome>(std::chrono::seconds(2)));

    bob->send(pp::ClientMessage { pp::Hello { "Bob" } });
    REQUIRE(bob_msgs.wait_for<pp::Welcome>(std::chrono::seconds(2)));

    alice->send(pp::ClientMessage { pp::CreateRoom { "Integration", 4 } });
    REQUIRE(alice_msgs.wait_for<pp::JoinedRoom>(std::chrono::seconds(2)));

    const auto joined = alice_msgs.find<pp::JoinedRoom>();
    REQUIRE(joined.has_value());
    const uint64_t room_id = joined->room_id;

    bob->send(pp::ClientMessage { pp::JoinRoom { room_id } });
    REQUIRE(bob_msgs.wait_for<pp::JoinedRoom>(std::chrono::seconds(2)));

    alice_msgs.clear();
    bob_msgs.clear();

    alice->send(pp::ClientMessage { pp::StartGame {} });
    REQUIRE(alice_msgs.wait_for<pp::GameStateUpdate>(std::chrono::seconds(2)));
    REQUIRE(bob_msgs.wait_for<pp::GameStateUpdate>(std::chrono::seconds(2)));

    client_guard.reset();
    client_io.stop();
    client_thread.join();

    server_guard.reset();
    server_io.stop();
    server_thread.join();
}
