#include "message_collector.h"
#include "network/server.h"

#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <catch_amalgamated.hpp>
#include <poker/game_constants.h>
#include <poker/protocol.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>

namespace pp = poker::protocol;

using namespace poker::client;
using poker::test::MessageCollector;

namespace {

std::string unique_integration_db_path()
{
    return (std::filesystem::temp_directory_path()
        / ("ascii_poker_integration_"
            + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
            + ".db"))
        .string();
}

} // namespace

TEST_CASE("Two TCP clients can create a room and start a game", "[integration]")
{
    constexpr unsigned short kPort = 19987;

    asio::io_context server_io;
    auto server_guard = asio::make_work_guard(server_io);
    poker::server::Server server(server_io, kPort, unique_integration_db_path());
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

    alice->send(pp::ClientMessage { pp::Login { "Alice", "pass1234" } });
    REQUIRE(alice_msgs.wait_for<pp::Welcome>(std::chrono::seconds(2)));

    bob->send(pp::ClientMessage { pp::Login { "Bob", "pass1234" } });
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

TEST_CASE("Server persists chip balance to database after blinds are posted", "[integration]")
{
    constexpr unsigned short kPort = 19988;

    asio::io_context server_io;
    auto server_guard = asio::make_work_guard(server_io);
    poker::server::Server server(server_io, kPort, unique_integration_db_path());
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

    alice->send(pp::ClientMessage { pp::Login { "Alice", "pass1234" } });
    REQUIRE(alice_msgs.wait_for<pp::Welcome>(std::chrono::seconds(2)));

    bob->send(pp::ClientMessage { pp::Login { "Bob", "pass1234" } });
    REQUIRE(bob_msgs.wait_for<pp::Welcome>(std::chrono::seconds(2)));

    alice->send(pp::ClientMessage { pp::CreateRoom { "Chips", 4 } });
    REQUIRE(alice_msgs.wait_for<pp::JoinedRoom>(std::chrono::seconds(2)));

    const auto joined = alice_msgs.find<pp::JoinedRoom>();
    REQUIRE(joined.has_value());

    bob->send(pp::ClientMessage { pp::JoinRoom { joined->room_id } });
    REQUIRE(bob_msgs.wait_for<pp::JoinedRoom>(std::chrono::seconds(2)));

    alice_msgs.clear();
    bob_msgs.clear();

    alice->send(pp::ClientMessage { pp::StartGame {} });
    REQUIRE(alice_msgs.wait_for<pp::GameStateUpdate>(std::chrono::seconds(2)));

    const auto alice_db = server.users().find_by_username("Alice");
    const auto bob_db = server.users().find_by_username("Bob");
    REQUIRE(alice_db.has_value());
    REQUIRE(bob_db.has_value());
    REQUIRE(alice_db->chips + bob_db->chips == 2000 - poker::SMALL_BLIND - poker::BIG_BLIND);

    client_guard.reset();
    client_io.stop();
    client_thread.join();

    server_guard.reset();
    server_io.stop();
    server_thread.join();
}
