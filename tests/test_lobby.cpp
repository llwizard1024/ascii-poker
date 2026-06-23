#include "game/lobby.h"
#include "mock_connection.h"
#include "network/connection_registry.h"
#include "poker/error_codes.h"
#include "poker/protocol.h"

#include <catch_amalgamated.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace poker::server;
using poker::test::MockConnection;
using poker::test::find_message;
using poker::test::has_message;

namespace {

std::shared_ptr<Lobby> make_lobby(std::shared_ptr<ConnectionRegistry>& registry_out)
{
    auto registry = std::make_shared<ConnectionRegistry>();
    registry_out = registry;
    return std::make_shared<Lobby>(registry);
}

std::optional<poker::protocol::RoomInfo> find_room(const poker::protocol::RoomList& list, uint64_t room_id)
{
    for (const auto& room : list.rooms) {
        if (room.room_id == room_id) {
            return room;
        }
    }
    return std::nullopt;
}

} // namespace

TEST_CASE("Creating a room registers the creator as host", "[lobby]")
{
    std::shared_ptr<ConnectionRegistry> registry;
    auto lobby = make_lobby(registry);

    auto host = std::make_shared<MockConnection>("Alice");
    registry->add(host);

    const auto room = lobby->create_room("Table 1", 4, host);
    REQUIRE(room != nullptr);
    REQUIRE(room->player_count() == 1);

    const auto joined = find_message<poker::protocol::JoinedRoom>(*host);
    REQUIRE(joined.has_value());
    REQUIRE(joined->host_name == "Alice");
    REQUIRE(joined->max_players == 4);
}

TEST_CASE("Room list is broadcast when a room is created", "[lobby]")
{
    std::shared_ptr<ConnectionRegistry> registry;
    auto lobby = make_lobby(registry);

    auto alice = std::make_shared<MockConnection>("Alice");
    auto bob = std::make_shared<MockConnection>("Bob");
    registry->add(alice);
    registry->add(bob);

    lobby->create_room("Table 1", 4, alice);

    REQUIRE(has_message<poker::protocol::RoomList>(*alice));
    REQUIRE(has_message<poker::protocol::RoomList>(*bob));

    const auto list = find_message<poker::protocol::RoomList>(*bob);
    REQUIRE(list.has_value());
    REQUIRE(list->rooms.size() == 1);
    REQUIRE(list->rooms[0].room_name == "Table 1");
    REQUIRE_FALSE(list->rooms[0].in_game);
}

TEST_CASE("Joining a room updates player count and broadcasts", "[lobby]")
{
    std::shared_ptr<ConnectionRegistry> registry;
    auto lobby = make_lobby(registry);

    auto host = std::make_shared<MockConnection>("Alice");
    auto guest = std::make_shared<MockConnection>("Bob");
    registry->add(host);
    registry->add(guest);

    const auto room = lobby->create_room("Table 1", 4, host);
    guest->clear_messages();

    const auto error = lobby->join_room(room->get_id(), guest);
    REQUIRE_FALSE(error.has_value());
    REQUIRE(room->player_count() == 2);

    const auto joined = find_message<poker::protocol::JoinedRoom>(*guest);
    REQUIRE(joined.has_value());
    REQUIRE(joined->player_names.size() == 2);
    REQUIRE(has_message<poker::protocol::RoomList>(*host));
}

TEST_CASE("Joining a non-existent room returns JoinFailed", "[lobby]")
{
    std::shared_ptr<ConnectionRegistry> registry;
    auto lobby = make_lobby(registry);

    auto player = std::make_shared<MockConnection>("Alice");
    registry->add(player);

    const auto error = lobby->join_room(999, player);
    REQUIRE(error.has_value());
    REQUIRE(*error == poker::protocol::ErrorCode::JoinFailed);
}

TEST_CASE("Host can start a game with two players", "[lobby]")
{
    std::shared_ptr<ConnectionRegistry> registry;
    auto lobby = make_lobby(registry);

    auto host = std::make_shared<MockConnection>("Alice");
    auto guest = std::make_shared<MockConnection>("Bob");
    registry->add(host);
    registry->add(guest);

    const auto room = lobby->create_room("Table 1", 4, host);
    REQUIRE_FALSE(lobby->join_room(room->get_id(), guest).has_value());

    host->clear_messages();
    guest->clear_messages();

    const auto error = lobby->start_game(host);
    REQUIRE_FALSE(error.has_value());
    REQUIRE(room->is_game_started());
    REQUIRE(has_message<poker::protocol::GameStateUpdate>(*host));
    REQUIRE(has_message<poker::protocol::GameStateUpdate>(*guest));

    const auto list = find_message<poker::protocol::RoomList>(*host);
    REQUIRE(list.has_value());
    const auto info = find_room(*list, room->get_id());
    REQUIRE(info.has_value());
    REQUIRE(info->in_game);
}

TEST_CASE("Non-host cannot start the game", "[lobby]")
{
    std::shared_ptr<ConnectionRegistry> registry;
    auto lobby = make_lobby(registry);

    auto host = std::make_shared<MockConnection>("Alice");
    auto guest = std::make_shared<MockConnection>("Bob");
    registry->add(host);
    registry->add(guest);

    const auto room = lobby->create_room("Table 1", 4, host);
    REQUIRE_FALSE(lobby->join_room(room->get_id(), guest).has_value());

    const auto error = lobby->start_game(guest);
    REQUIRE(error.has_value());
    REQUIRE(*error == poker::protocol::ErrorCode::NotRoomHost);
    REQUIRE_FALSE(room->is_game_started());
}

TEST_CASE("Starting with one player returns NotEnoughPlayers", "[lobby]")
{
    std::shared_ptr<ConnectionRegistry> registry;
    auto lobby = make_lobby(registry);

    auto host = std::make_shared<MockConnection>("Alice");
    registry->add(host);

    lobby->create_room("Table 1", 4, host);

    const auto error = lobby->start_game(host);
    REQUIRE(error.has_value());
    REQUIRE(*error == poker::protocol::ErrorCode::NotEnoughPlayers);
}

TEST_CASE("Joining a room after the game started returns GameAlreadyStarted", "[lobby]")
{
    std::shared_ptr<ConnectionRegistry> registry;
    auto lobby = make_lobby(registry);

    auto host = std::make_shared<MockConnection>("Alice");
    auto guest = std::make_shared<MockConnection>("Bob");
    auto latecomer = std::make_shared<MockConnection>("Charlie");
    registry->add(host);
    registry->add(guest);
    registry->add(latecomer);

    const auto room = lobby->create_room("Table 1", 4, host);
    REQUIRE_FALSE(lobby->join_room(room->get_id(), guest).has_value());
    REQUIRE_FALSE(lobby->start_game(host).has_value());

    const auto error = lobby->join_room(room->get_id(), latecomer);
    REQUIRE(error.has_value());
    REQUIRE(*error == poker::protocol::ErrorCode::GameAlreadyStarted);
}
