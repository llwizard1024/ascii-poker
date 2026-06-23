#include "poker/protocol.h"
#include <catch_amalgamated.hpp>
#include <stdexcept>
#include <string>
#include <vector>

using namespace poker;
using namespace poker::protocol;

template <typename T>
void test_roundtrip(const T& original)
{
    nlohmann::json j = original;
    T deserialized = j.get<T>();
    REQUIRE(nlohmann::json(original) == nlohmann::json(deserialized));
}

TEST_CASE("Hello round-trip", "[protocol]")
{
    Hello msg { "Alice" };
    test_roundtrip(msg);
}

TEST_CASE("Welcome round-trip", "[protocol]")
{
    Welcome msg { "Bob" };
    test_roundtrip(msg);
}

TEST_CASE("CreateRoom round-trip", "[protocol]")
{
    CreateRoom msg { "Test Room", 6 };
    test_roundtrip(msg);
}

TEST_CASE("JoinRoom round-trip", "[protocol]")
{
    JoinRoom msg { 42 };
    test_roundtrip(msg);
}

TEST_CASE("LeaveRoom round-trip", "[protocol]")
{
    LeaveRoom msg;
    test_roundtrip(msg);
}

TEST_CASE("ListRooms round-trip", "[protocol]")
{
    ListRooms msg;
    test_roundtrip(msg);
}

TEST_CASE("LeftRoom round-trip", "[protocol]")
{
    LeftRoom msg { 42 };
    test_roundtrip(msg);
}

TEST_CASE("HandResult round-trip", "[protocol]")
{
    HandResult msg { { "Alice", "Bob" }, 1500 };
    test_roundtrip(msg);
}

TEST_CASE("Error round-trip", "[protocol]")
{
    Error msg { 404, "Room not found" };
    test_roundtrip(msg);
}

TEST_CASE("PlayerAction with nullopt amount (Fold)", "[protocol]")
{
    PlayerAction msg { Action::Fold, std::nullopt };
    nlohmann::json j = msg;
    REQUIRE(j["action"] == "fold");
    REQUIRE(j["amount"].is_null());

    PlayerAction parsed = j.get<PlayerAction>();
    REQUIRE(parsed.action == Action::Fold);
    REQUIRE_FALSE(parsed.amount.has_value());
    REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
}

TEST_CASE("PlayerAction with concrete amount (Raise)", "[protocol]")
{
    PlayerAction msg { Action::Raise, 500 };
    nlohmann::json j = msg;
    REQUIRE(j["action"] == "raise");
    REQUIRE(j["amount"] == 500);

    PlayerAction parsed = j.get<PlayerAction>();
    REQUIRE(parsed.action == Action::Raise);
    REQUIRE(parsed.amount.has_value());
    REQUIRE(*parsed.amount == 500);
    REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
}

TEST_CASE("RoomInfo round-trip", "[protocol]")
{
    RoomInfo info { 10, "Lounge", 3, 8 };
    test_roundtrip(info);
}

TEST_CASE("RoomList with multiple rooms", "[protocol]")
{
    RoomList list;
    list.rooms.push_back(RoomInfo { 1, "A", 2, 5 });
    list.rooms.push_back(RoomInfo { 2, "B", 3, 6 });
    test_roundtrip(list);
}

TEST_CASE("JoinedRoom with non‑empty player names", "[protocol]")
{
    JoinedRoom msg { 7, { "Alice", "Bob", "Charlie" }, "Alice", 6 };
    test_roundtrip(msg);
}

TEST_CASE("GameStateUpdate with empty cards", "[protocol]")
{
    GameStateUpdate state;
    state.phase = GamePhase::PreFlop;
    state.active_player_name = "Alice";
    state.total_pot = 200;

    test_roundtrip(state);
}

TEST_CASE("GameStateUpdate with non‑empty cards", "[protocol]")
{
    GameStateUpdate state;
    state.general_cards = {
        Card(Suit::Hearts, Rank::Ace),
        Card(Suit::Clubs, Rank::Ten),
        Card(Suit::Diamonds, Rank::Five)
    };
    state.your_cards = {
        Card(Suit::Spades, Rank::King),
        Card(Suit::Spades, Rank::Queen)
    };
    state.phase = GamePhase::Flop;
    state.active_player_name = "Bob";
    state.total_pot = 750;

    nlohmann::json j = state;
    GameStateUpdate parsed = j.get<GameStateUpdate>();

    REQUIRE(parsed.general_cards.size() == state.general_cards.size());
    REQUIRE(parsed.general_cards == state.general_cards);
    REQUIRE(parsed.your_cards == state.your_cards);
    REQUIRE(parsed.phase == state.phase);
    REQUIRE(parsed.active_player_name == state.active_player_name);
    REQUIRE(parsed.total_pot == state.total_pot);
    REQUIRE(nlohmann::json(state) == nlohmann::json(parsed));
}

TEST_CASE("YourTurn with available actions", "[protocol]")
{
    YourTurn turn;
    turn.available_actions = { Action::Fold, Action::Check, Action::Raise };
    turn.min_amount = 10;
    turn.max_amount = 200;
    test_roundtrip(turn);
}

TEST_CASE("ClientMessage serialization for all variants", "[protocol]")
{
    SECTION("Hello")
    {
        ClientMessage msg = Hello { "Alice" };
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "hello");
        ClientMessage parsed = j.get<ClientMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("CreateRoom")
    {
        ClientMessage msg = CreateRoom { "Test", 4 };
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "create_room");
        ClientMessage parsed = j.get<ClientMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("JoinRoom")
    {
        ClientMessage msg = JoinRoom { 123 };
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "join_room");
        ClientMessage parsed = j.get<ClientMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("LeaveRoom")
    {
        ClientMessage msg = LeaveRoom {};
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "leave_room");
        ClientMessage parsed = j.get<ClientMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("ListRooms")
    {
        ClientMessage msg = ListRooms {};
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "list_rooms");
        ClientMessage parsed = j.get<ClientMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("StartGame")
    {
        ClientMessage msg = StartGame {};
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "start_game");
        ClientMessage parsed = j.get<ClientMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("PlayerAction")
    {
        ClientMessage msg = PlayerAction { Action::Raise, 100 };
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "player_action");
        ClientMessage parsed = j.get<ClientMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
}

TEST_CASE("ServerMessage serialization for all variants", "[protocol]")
{
    SECTION("Welcome")
    {
        ServerMessage msg = Welcome { "Alice" };
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "welcome");
        ServerMessage parsed = j.get<ServerMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("RoomList")
    {
        ServerMessage msg = RoomList { { RoomInfo { 1, "Room1", 0, 4 } } };
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "room_list");
        ServerMessage parsed = j.get<ServerMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("JoinedRoom")
    {
        ServerMessage msg = JoinedRoom { 5, { "X", "Y" }, "X", 4 };
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "joined_room");
        ServerMessage parsed = j.get<ServerMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("GameStateUpdate")
    {
        GameStateUpdate gsu;
        gsu.phase = GamePhase::Turn;
        gsu.active_player_name = "P";
        gsu.total_pot = 300;
        ServerMessage msg = gsu;
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "game_state_update");
        ServerMessage parsed = j.get<ServerMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("YourTurn")
    {
        YourTurn yt;
        yt.available_actions = { Action::Call, Action::Fold };
        yt.min_amount = 0;
        yt.max_amount = 500;
        ServerMessage msg = yt;
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "your_turn");
        ServerMessage parsed = j.get<ServerMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("Error")
    {
        ServerMessage msg = Error { 1, "bad" };
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "error");
        ServerMessage parsed = j.get<ServerMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("LeftRoom")
    {
        ServerMessage msg = LeftRoom { 3 };
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "left_room");
        ServerMessage parsed = j.get<ServerMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
    SECTION("HandResult")
    {
        ServerMessage msg = HandResult { { "Winner" }, 900 };
        nlohmann::json j = msg;
        REQUIRE(j["type"] == "hand_result");
        ServerMessage parsed = j.get<ServerMessage>();
        REQUIRE(nlohmann::json(msg) == nlohmann::json(parsed));
    }
}

TEST_CASE("Unknown ClientMessage type throws", "[protocol]")
{
    nlohmann::json j = {
        { "type", "unknown" },
        { "data", nlohmann::json::object() }
    };
    REQUIRE_THROWS_AS(j.get<ClientMessage>(), std::invalid_argument);
}