#include "game/game_session.h"
#include "game/remote_player.h"
#include "mock_connection.h"
#include "poker/game_constants.h"
#include "poker/protocol.h"

#include <catch_amalgamated.hpp>

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

using namespace poker::server;
using poker::test::MockConnection;
using poker::test::find_message;
using poker::test::find_last_message;

namespace {

std::shared_ptr<IPlayer> make_remote(const std::shared_ptr<MockConnection>& connection)
{
    return std::make_shared<RemotePlayer>(connection);
}

bool has_hand_result(const MockConnection& connection)
{
    return poker::test::has_message<poker::protocol::HandResult>(connection);
}

GameSession make_session(const std::vector<std::shared_ptr<MockConnection>>& connections)
{
    std::vector<std::shared_ptr<IPlayer>> players;
    std::unordered_map<IConnection*, IPlayer*> connection_map;

    for (const auto& connection : connections) {
        auto player = make_remote(connection);
        players.push_back(player);
        connection_map[connection.get()] = player.get();
    }

    return GameSession(std::move(players), std::move(connection_map), 1);
}

} // namespace

TEST_CASE("Blinds are posted at hand start", "[game_session]")
{
    auto alice_conn = std::make_shared<MockConnection>("Alice");
    auto bob_conn = std::make_shared<MockConnection>("Bob");

    GameSession session = make_session({ alice_conn, bob_conn });
    session.start();

    REQUIRE(session.pot_total() == poker::SMALL_BLIND + poker::BIG_BLIND);
}

TEST_CASE("Removing player during heads-up awards pot and ends session viability", "[game_session]")
{
    auto alice_conn = std::make_shared<MockConnection>("Alice");
    auto bob_conn = std::make_shared<MockConnection>("Bob");

    GameSession session = make_session({ alice_conn, bob_conn });
    session.start();

    session.remove_player(alice_conn);

    REQUIRE(session.player_count() == 1);
    REQUIRE_FALSE(session.has_enough_players());
    REQUIRE(has_hand_result(*bob_conn));
}

TEST_CASE("Removing one of three players keeps session alive", "[game_session]")
{
    auto a_conn = std::make_shared<MockConnection>("A");
    auto b_conn = std::make_shared<MockConnection>("B");
    auto c_conn = std::make_shared<MockConnection>("C");

    GameSession session = make_session({ a_conn, b_conn, c_conn });
    session.start();

    session.remove_player(c_conn);

    REQUIRE(session.player_count() == 2);
    REQUIRE(session.has_enough_players());
}

TEST_CASE("Minimum raise follows big blind increment after blinds", "[game_session]")
{
    auto alice_conn = std::make_shared<MockConnection>("Alice");
    auto bob_conn = std::make_shared<MockConnection>("Bob");

    GameSession session = make_session({ alice_conn, bob_conn });
    session.start();

    const auto turn = find_message<poker::protocol::YourTurn>(*alice_conn);
    REQUIRE(turn.has_value());
    REQUIRE(turn->min_amount == poker::BIG_BLIND + poker::BIG_BLIND);
    REQUIRE(poker::test::has_message<poker::protocol::YourTurn>(*alice_conn));
}

TEST_CASE("Heads-up BB gets option after SB completes call", "[game_session]")
{
    auto alice_conn = std::make_shared<MockConnection>("Alice");
    auto bob_conn = std::make_shared<MockConnection>("Bob");

    GameSession session = make_session({ alice_conn, bob_conn });
    session.start();

    const auto alice_turn = find_message<poker::protocol::YourTurn>(*alice_conn);
    REQUIRE(alice_turn.has_value());
    session.apply_action(alice_conn, poker::protocol::Action::Call, std::nullopt);

    const auto bob_turn = find_last_message<poker::protocol::YourTurn>(*bob_conn);
    REQUIRE(bob_turn.has_value());

    const auto state = find_last_message<poker::protocol::GameStateUpdate>(*alice_conn);
    REQUIRE(state.has_value());
    REQUIRE(state->phase == poker::protocol::GamePhase::PreFlop);

    session.apply_action(bob_conn, poker::protocol::Action::Check, std::nullopt);

    const auto flop_state = find_last_message<poker::protocol::GameStateUpdate>(*alice_conn);
    REQUIRE(flop_state.has_value());
    REQUIRE(flop_state->phase == poker::protocol::GamePhase::Flop);
}

TEST_CASE("Three players must all act before flop after limping", "[game_session]")
{
    auto a_conn = std::make_shared<MockConnection>("A");
    auto b_conn = std::make_shared<MockConnection>("B");
    auto c_conn = std::make_shared<MockConnection>("C");

    GameSession session = make_session({ a_conn, b_conn, c_conn });
    session.start();

    const auto a_turn = find_message<poker::protocol::YourTurn>(*a_conn);
    REQUIRE(a_turn.has_value());
    session.apply_action(a_conn, poker::protocol::Action::Call, std::nullopt);

    const auto b_turn = find_last_message<poker::protocol::YourTurn>(*b_conn);
    REQUIRE(b_turn.has_value());
    session.apply_action(b_conn, poker::protocol::Action::Call, std::nullopt);

    a_conn->clear_messages();
    REQUIRE_FALSE(poker::test::has_message<poker::protocol::GameStateUpdate>(*a_conn));

    const auto c_turn = find_last_message<poker::protocol::YourTurn>(*c_conn);
    REQUIRE(c_turn.has_value());
    session.apply_action(c_conn, poker::protocol::Action::Check, std::nullopt);

    const auto flop_state = find_last_message<poker::protocol::GameStateUpdate>(*a_conn);
    REQUIRE(flop_state.has_value());
    REQUIRE(flop_state->phase == poker::protocol::GamePhase::Flop);
}

TEST_CASE("Three-way flop requires three checks to advance", "[game_session]")
{
    auto a_conn = std::make_shared<MockConnection>("A");
    auto b_conn = std::make_shared<MockConnection>("B");
    auto c_conn = std::make_shared<MockConnection>("C");

    GameSession session = make_session({ a_conn, b_conn, c_conn });
    session.start();

    const auto a_preflop = find_message<poker::protocol::YourTurn>(*a_conn);
    REQUIRE(a_preflop.has_value());
    session.apply_action(a_conn, poker::protocol::Action::Call, std::nullopt);

    const auto b_preflop = find_message<poker::protocol::YourTurn>(*b_conn);
    REQUIRE(b_preflop.has_value());
    session.apply_action(b_conn, poker::protocol::Action::Call, std::nullopt);

    const auto c_preflop = find_message<poker::protocol::YourTurn>(*c_conn);
    REQUIRE(c_preflop.has_value());
    session.apply_action(c_conn, poker::protocol::Action::Check, std::nullopt);

    session.apply_action(c_conn, poker::protocol::Action::Check, std::nullopt);

    const auto b_flop = find_last_message<poker::protocol::YourTurn>(*b_conn);
    REQUIRE(b_flop.has_value());
    session.apply_action(b_conn, poker::protocol::Action::Check, std::nullopt);

    const auto c_flop = find_last_message<poker::protocol::YourTurn>(*c_conn);
    REQUIRE(c_flop.has_value());
    session.apply_action(c_conn, poker::protocol::Action::Check, std::nullopt);

    const auto a_flop = find_last_message<poker::protocol::YourTurn>(*a_conn);
    REQUIRE(a_flop.has_value());
    session.apply_action(a_conn, poker::protocol::Action::Check, std::nullopt);

    const auto turn_state = find_last_message<poker::protocol::GameStateUpdate>(*a_conn);
    REQUIRE(turn_state.has_value());
    REQUIRE(turn_state->phase == poker::protocol::GamePhase::Turn);
}

TEST_CASE("Heads-up all-in preflop completes and starts next hand", "[game_session]")
{
    auto alice_conn = std::make_shared<MockConnection>("Alice");
    auto bob_conn = std::make_shared<MockConnection>("Bob");

    GameSession session = make_session({ alice_conn, bob_conn });
    session.start();

    const auto alice_turn = find_message<poker::protocol::YourTurn>(*alice_conn);
    REQUIRE(alice_turn.has_value());
    session.apply_action(alice_conn, poker::protocol::Action::Raise, alice_turn->max_amount);

    const auto bob_turn = find_message<poker::protocol::YourTurn>(*bob_conn);
    REQUIRE(bob_turn.has_value());
    session.apply_action(bob_conn, poker::protocol::Action::Call, std::nullopt);

    REQUIRE(has_hand_result(*alice_conn));
    REQUIRE(has_hand_result(*bob_conn));
    REQUIRE(session.has_enough_players());

    if (session.count_players_with_chips() >= 2) {
        alice_conn->clear_messages();
        bob_conn->clear_messages();
        REQUIRE_NOTHROW(session.apply_action(alice_conn, poker::protocol::Action::Check, std::nullopt));
    } else {
        REQUIRE(session.count_players_with_chips() == 1);
    }
}
