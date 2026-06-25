#include "game/game_session.h"
#include "game/lobby.h"
#include "game/remote_player.h"
#include "mock_connection.h"
#include "network/connection_registry.h"
#include "storage/database.h"
#include "storage/user_repository.h"

#include <catch_amalgamated.hpp>
#include <poker/game_constants.h>
#include <poker/protocol.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace poker::server;
using poker::test::MockConnection;
using poker::test::find_last_message;

namespace {

std::string unique_db_path()
{
    return (std::filesystem::temp_directory_path()
        / ("ascii_poker_chips_" + std::to_string(
               std::chrono::steady_clock::now().time_since_epoch().count())
            + ".db"))
        .string();
}

GameSession make_session_with_bankrolls(
    const std::vector<std::shared_ptr<MockConnection>>& connections,
    const std::unordered_map<std::string, uint32_t>& bankrolls,
    UserRepository* users = nullptr)
{
    std::vector<std::shared_ptr<IPlayer>> players;
    std::unordered_map<IConnection*, IPlayer*> connection_map;

    for (const auto& connection : connections) {
        auto player = std::make_shared<RemotePlayer>(connection);
        players.push_back(player);
        connection_map[connection.get()] = player.get();
    }

    return GameSession(std::move(players), std::move(connection_map), 1, users, bankrolls);
}

} // namespace

TEST_CASE("GameSession persists chips to database after blinds are posted", "[chips]")
{
    const std::string db_path = unique_db_path();
    Database database(db_path);
    UserRepository users(database);

    REQUIRE(users.create_user("Alice", "password123", 1000) == UserCreateResult::Created);
    REQUIRE(users.create_user("Bob", "password123", 1000) == UserCreateResult::Created);

    auto alice_conn = std::make_shared<MockConnection>("Alice");
    auto bob_conn = std::make_shared<MockConnection>("Bob");

    std::unordered_map<std::string, uint32_t> bankrolls;
    bankrolls["Alice"] = 1000;
    bankrolls["Bob"] = 1000;

    GameSession session = make_session_with_bankrolls({ alice_conn, bob_conn }, bankrolls, &users);
    session.start();

    const auto alice_db = users.find_by_username("Alice");
    const auto bob_db = users.find_by_username("Bob");
    REQUIRE(alice_db.has_value());
    REQUIRE(bob_db.has_value());
    REQUIRE(alice_db->chips + bob_db->chips == 2000 - poker::SMALL_BLIND - poker::BIG_BLIND);
}

TEST_CASE("GameSession uses bankrolls from database instead of default stack", "[chips]")
{
    auto alice_conn = std::make_shared<MockConnection>("Alice");
    auto bob_conn = std::make_shared<MockConnection>("Bob");

    GameSession session = make_session_with_bankrolls(
        { alice_conn, bob_conn },
        { { "Alice", 500 }, { "Bob", 800 } });
    session.start();

    const auto alice_state = find_last_message<poker::protocol::GameStateUpdate>(*alice_conn);
    REQUIRE(alice_state.has_value());
    REQUIRE(alice_state->players.size() == 2);

    const auto alice = std::find_if(
        alice_state->players.begin(),
        alice_state->players.end(),
        [](const poker::protocol::PlayerState& player) { return player.name == "Alice"; });
    const auto bob = std::find_if(
        alice_state->players.begin(),
        alice_state->players.end(),
        [](const poker::protocol::PlayerState& player) { return player.name == "Bob"; });

    REQUIRE(alice != alice_state->players.end());
    REQUIRE(bob != alice_state->players.end());
    REQUIRE(alice->chips == 500 - poker::SMALL_BLIND);
    REQUIRE(bob->chips == 800 - poker::BIG_BLIND);
}

TEST_CASE("GameSession persists chip totals after a completed hand", "[chips]")
{
    const std::string db_path = unique_db_path();
    Database database(db_path);
    UserRepository users(database);

    REQUIRE(users.create_user("Alice", "password123", 1000) == UserCreateResult::Created);
    REQUIRE(users.create_user("Bob", "password123", 1000) == UserCreateResult::Created);

    auto alice_conn = std::make_shared<MockConnection>("Alice");
    auto bob_conn = std::make_shared<MockConnection>("Bob");

    std::unordered_map<std::string, uint32_t> bankrolls;
    bankrolls["Alice"] = users.find_by_username("Alice")->chips;
    bankrolls["Bob"] = users.find_by_username("Bob")->chips;

    GameSession session = make_session_with_bankrolls({ alice_conn, bob_conn }, bankrolls, &users);
    session.start();

    const auto alice_turn = find_last_message<poker::protocol::YourTurn>(*alice_conn);
    REQUIRE(alice_turn.has_value());
    session.apply_action(alice_conn, poker::protocol::Action::Raise, alice_turn->max_amount);

    const auto bob_turn = find_last_message<poker::protocol::YourTurn>(*bob_conn);
    REQUIRE(bob_turn.has_value());
    session.apply_action(bob_conn, poker::protocol::Action::Call, std::nullopt);

    const auto alice_after = users.find_by_username("Alice");
    const auto bob_after = users.find_by_username("Bob");
    REQUIRE(alice_after.has_value());
    REQUIRE(bob_after.has_value());
    REQUIRE(alice_after->chips + bob_after->chips == 2000);
    REQUIRE((alice_after->chips == 2000 || bob_after->chips == 2000));
}

TEST_CASE("Heads-up fold win conserves chips and persists correct balances", "[chips]")
{
    const std::string db_path = unique_db_path();
    Database database(db_path);
    UserRepository users(database);

    REQUIRE(users.create_user("Oleg", "password123", 1000) == UserCreateResult::Created);
    REQUIRE(users.create_user("Bogdan", "password123", 1000) == UserCreateResult::Created);

    auto oleg_conn = std::make_shared<MockConnection>("Oleg");
    auto bogdan_conn = std::make_shared<MockConnection>("Bogdan");

    std::unordered_map<std::string, uint32_t> bankrolls;
    bankrolls["Oleg"] = 1000;
    bankrolls["Bogdan"] = 1000;

    GameSession session = make_session_with_bankrolls({ oleg_conn, bogdan_conn }, bankrolls, &users);
    session.start();

    const auto oleg_turn = find_last_message<poker::protocol::YourTurn>(*oleg_conn);
    REQUIRE(oleg_turn.has_value());
    session.apply_action(oleg_conn, poker::protocol::Action::Call, std::nullopt);

    const auto bogdan_turn = find_last_message<poker::protocol::YourTurn>(*bogdan_conn);
    REQUIRE(bogdan_turn.has_value());
    session.apply_action(bogdan_conn, poker::protocol::Action::Fold, std::nullopt);

    uint32_t oleg_stack_after_win = 0;
    uint32_t bogdan_stack_after_win = 0;
    for (const auto& msg : oleg_conn->sent_messages()) {
        if (std::holds_alternative<poker::protocol::GameStateUpdate>(msg)) {
            const auto& update = std::get<poker::protocol::GameStateUpdate>(msg);
            for (const auto& player : update.players) {
                if (player.name == "Oleg") {
                    oleg_stack_after_win = player.chips;
                }
                if (player.name == "Bogdan") {
                    bogdan_stack_after_win = player.chips;
                }
            }
        }
        if (std::holds_alternative<poker::protocol::HandResult>(msg)) {
            break;
        }
    }

    REQUIRE(oleg_stack_after_win == 1020);
    REQUIRE(bogdan_stack_after_win == 980);
    REQUIRE(oleg_stack_after_win + bogdan_stack_after_win == 2000);

    const auto oleg_db = users.find_by_username("Oleg");
    const auto bogdan_db = users.find_by_username("Bogdan");
    REQUIRE(oleg_db.has_value());
    REQUIRE(bogdan_db.has_value());
    REQUIRE(oleg_db->chips + bogdan_db->chips == 2000 - poker::SMALL_BLIND - poker::BIG_BLIND);
}

TEST_CASE("Room rejects game start when a player has insufficient chips", "[chips]")
{
    const std::string db_path = unique_db_path();
    Database database(db_path);
    UserRepository users(database);

    REQUIRE(users.create_user("Host", "password123", 1000) == UserCreateResult::Created);
    REQUIRE(users.create_user("Guest", "password123", 1000) == UserCreateResult::Created);

    auto registry = std::make_shared<ConnectionRegistry>();
    auto lobby = std::make_shared<Lobby>(registry, &users);

    auto host = std::make_shared<MockConnection>("Host");
    auto guest = std::make_shared<MockConnection>("Guest");
    host->set_player_name("Host");
    guest->set_player_name("Guest");
    registry->add(host);
    registry->add(guest);

    lobby->create_room("Table", 4, host);
    REQUIRE(lobby->join_room(1, guest) == std::nullopt);
    REQUIRE(users.update_chips("Guest", poker::BIG_BLIND - 1));

    const auto start_error = lobby->start_game(host);
    REQUIRE(start_error.has_value());
    REQUIRE(*start_error == poker::protocol::ErrorCode::NotEnoughChips);
}
