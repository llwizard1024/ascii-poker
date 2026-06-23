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
