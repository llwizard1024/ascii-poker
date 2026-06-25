#include "game/lobby.h"
#include "messaging/handshake_message_processor.h"
#include "mock_connection.h"
#include "network/connection_registry.h"
#include "network/player_name_registry.h"
#include "storage/database.h"
#include "storage/user_repository.h"

#include <catch_amalgamated.hpp>
#include <json/json.hpp>
#include <poker/game_constants.h>
#include <poker/protocol.h>

#include <chrono>
#include <filesystem>
#include <memory>

using namespace poker::server;
using poker::test::find_last_message;
using poker::test::MockConnection;

namespace {

std::string unique_db_path()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return (std::filesystem::temp_directory_path() / ("ascii_poker_login_test_" + std::to_string(now) + ".db")).string();
}

std::shared_ptr<HandshakeMessageProcessor> make_processor(UserRepository& users)
{
    auto registry = std::make_shared<ConnectionRegistry>();
    auto lobby = std::make_shared<Lobby>(registry);
    auto names = std::make_shared<PlayerNameRegistry>();
    return std::make_shared<HandshakeMessageProcessor>(lobby, names, registry, &users);
}

std::string login_json(const std::string& username, const std::string& password)
{
    const poker::protocol::Login login { username, password };
    nlohmann::json envelope;
    envelope["type"] = "login";
    envelope["data"] = login;
    return envelope.dump();
}

void send_login(HandshakeMessageProcessor& processor, ConnectionPtr connection, const std::string& json)
{
    processor.process_message(json, connection);
}

} // namespace

TEST_CASE("Login registers a new account", "[auth]")
{
    Database database(unique_db_path());
    UserRepository users(database);
    auto processor = make_processor(users);
    auto connection = std::make_shared<MockConnection>("");

    send_login(*processor, connection, login_json("Alice", "password123"));

    const auto welcome = find_last_message<poker::protocol::Welcome>(*connection);
    REQUIRE(welcome.has_value());
    REQUIRE(welcome->player_name == "Alice");
    REQUIRE(welcome->chips == poker::STARTING_CHIPS);
    REQUIRE(welcome->is_new_account);
    REQUIRE(connection->is_authenticated());
}

TEST_CASE("Login accepts existing user with correct password", "[auth]")
{
    Database database(unique_db_path());
    UserRepository users(database);
    REQUIRE(users.create_user("Bob", "pw1234", 850) == UserCreateResult::Created);

    auto processor = make_processor(users);
    auto connection = std::make_shared<MockConnection>("");

    send_login(*processor, connection, login_json("Bob", "pw1234"));

    const auto welcome = find_last_message<poker::protocol::Welcome>(*connection);
    REQUIRE(welcome.has_value());
    REQUIRE(welcome->chips == 850);
    REQUIRE_FALSE(welcome->is_new_account);
}

TEST_CASE("Login rejects wrong password", "[auth]")
{
    Database database(unique_db_path());
    UserRepository users(database);
    REQUIRE(users.create_user("Carol", "correct", 1000) == UserCreateResult::Created);

    auto processor = make_processor(users);
    auto connection = std::make_shared<MockConnection>("");

    send_login(*processor, connection, login_json("Carol", "wrong1"));

    const auto error = find_last_message<poker::protocol::Error>(*connection);
    REQUIRE(error.has_value());
    REQUIRE(error->code == static_cast<int>(poker::protocol::ErrorCode::WrongPassword));
    REQUIRE_FALSE(connection->is_authenticated());
}

TEST_CASE("Login rejects already online account", "[auth]")
{
    Database database(unique_db_path());
    UserRepository users(database);
    REQUIRE(users.create_user("Dave", "pw1234", 1000) == UserCreateResult::Created);

    auto processor = make_processor(users);
    auto first = std::make_shared<MockConnection>("");
    auto second = std::make_shared<MockConnection>("");

    send_login(*processor, first, login_json("Dave", "pw1234"));
    REQUIRE(find_last_message<poker::protocol::Welcome>(*first).has_value());

    send_login(*processor, second, login_json("Dave", "pw1234"));

    const auto error = find_last_message<poker::protocol::Error>(*second);
    REQUIRE(error.has_value());
    REQUIRE(error->code == static_cast<int>(poker::protocol::ErrorCode::AlreadyOnline));
    REQUIRE_FALSE(second->is_authenticated());
}

TEST_CASE("Login rejects invalid password format", "[auth]")
{
    Database database(unique_db_path());
    UserRepository users(database);
    auto processor = make_processor(users);
    auto connection = std::make_shared<MockConnection>("");

    send_login(*processor, connection, login_json("Eve", "abc"));

    const auto error = find_last_message<poker::protocol::Error>(*connection);
    REQUIRE(error.has_value());
    REQUIRE(error->code == static_cast<int>(poker::protocol::ErrorCode::InvalidPassword));
}
