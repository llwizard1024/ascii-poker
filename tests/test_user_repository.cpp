#include "storage/password_hash.h"
#include "storage/user_repository.h"

#include <catch_amalgamated.hpp>

#include <chrono>
#include <filesystem>
#include <string>

using namespace poker::server;

namespace {

std::string unique_db_path()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return (std::filesystem::temp_directory_path() / ("ascii_poker_test_" + std::to_string(now) + ".db")).string();
}

} // namespace

TEST_CASE("PasswordHasher verifies stored hash", "[storage]")
{
    const std::string hash = PasswordHasher::hash("secret123");
    REQUIRE(PasswordHasher::verify("secret123", hash));
    REQUIRE_FALSE(PasswordHasher::verify("wrong", hash));
}

TEST_CASE("UserRepository creates and loads users", "[storage]")
{
    Database database(unique_db_path());
    UserRepository users(database);

    REQUIRE(users.create_user("Alice", "password", 1500) == UserCreateResult::Created);
    REQUIRE(users.create_user("Alice", "other", 1500) == UserCreateResult::UsernameTaken);

    const auto record = users.find_by_username("alice");
    REQUIRE(record.has_value());
    REQUIRE(record->username == "Alice");
    REQUIRE(record->chips == 1500);
    REQUIRE(users.verify_password("Alice", "password"));
    REQUIRE_FALSE(users.verify_password("Alice", "wrong"));
}

TEST_CASE("UserRepository updates chips", "[storage]")
{
    Database database(unique_db_path());
    UserRepository users(database);

    REQUIRE(users.create_user("Bob", "pw", 1000) == UserCreateResult::Created);
    REQUIRE(users.update_chips("Bob", 750));
    REQUIRE(users.find_by_username("Bob")->chips == 750);
}
