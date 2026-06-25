#include "poker/auth.h"

#include <catch_amalgamated.hpp>

TEST_CASE("Password validation requires length, letter, and digit", "[auth]")
{
    REQUIRE_FALSE(poker::auth::is_valid_password("abc"));
    REQUIRE_FALSE(poker::auth::is_valid_password("1234"));
    REQUIRE_FALSE(poker::auth::is_valid_password("abcd"));
    REQUIRE(poker::auth::is_valid_password("abc1"));
    REQUIRE(poker::auth::is_valid_password("password123"));
}

TEST_CASE("Username validation rejects control characters and length", "[auth]")
{
    REQUIRE(poker::auth::is_valid_username("Alice"));
    REQUIRE_FALSE(poker::auth::is_valid_username(""));
    REQUIRE_FALSE(poker::auth::is_valid_username(std::string(33, 'a')));
}

TEST_CASE("trim_copy removes surrounding whitespace", "[auth]")
{
    REQUIRE(poker::auth::trim_copy("  Alice  ") == "Alice");
    REQUIRE(poker::auth::trim_copy("\t\n").empty());
}
