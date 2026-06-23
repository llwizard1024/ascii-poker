#include "network/player_name_registry.h"

#include <catch_amalgamated.hpp>

using namespace poker::server;

TEST_CASE("PlayerNameRegistry tracks unique names", "[player_names]")
{
    PlayerNameRegistry registry;

    REQUIRE(registry.try_register("Alice"));
    REQUIRE_FALSE(registry.try_register("Alice"));
    REQUIRE(registry.is_taken("Alice"));
    REQUIRE(registry.try_register("Bob"));

    registry.unregister("Alice");
    REQUIRE_FALSE(registry.is_taken("Alice"));
    REQUIRE(registry.try_register("Alice"));
}
