#include "poker/side_pots.h"

#include <catch_amalgamated.hpp>

using namespace poker;

TEST_CASE("Single main pot when all contributions are equal", "[side_pots]")
{
    const std::vector<PotPlayerState> players = {
        { 100, false },
        { 100, false },
        { 100, true },
    };

    const auto pots = compute_side_pots(players);

    REQUIRE(pots.size() == 1);
    REQUIRE(pots[0].amount == 300);
    REQUIRE(pots[0].eligible_player_indices == std::vector<size_t> { 0, 1 });
}

TEST_CASE("Side pot splits short all-in from larger stacks", "[side_pots]")
{
    const std::vector<PotPlayerState> players = {
        { 100, false },
        { 200, false },
        { 50, true },
    };

    const auto pots = compute_side_pots(players);

    REQUIRE(pots.size() == 2);
    REQUIRE(pots[0].amount == 250);
    REQUIRE(pots[0].eligible_player_indices == std::vector<size_t> { 0, 1 });
    REQUIRE(pots[1].amount == 100);
    REQUIRE(pots[1].eligible_player_indices == std::vector<size_t> { 1 });
}

TEST_CASE("Folded players contribute but are not eligible", "[side_pots]")
{
    const std::vector<PotPlayerState> players = {
        { 50, true },
        { 50, false },
    };

    const auto pots = compute_side_pots(players);

    REQUIRE(pots.size() == 1);
    REQUIRE(pots[0].amount == 100);
    REQUIRE(pots[0].eligible_player_indices == std::vector<size_t> { 1 });
}

TEST_CASE("One active player contributed while opponent has zero", "[side_pots]")
{
    const std::vector<PotPlayerState> players = {
        { 20, false },
        { 0, false },
    };

    const auto pots = compute_side_pots(players);

    REQUIRE(pots.size() == 1);
    REQUIRE(pots[0].amount == 20);
    REQUIRE(pots[0].eligible_player_indices == std::vector<size_t> { 0, 1 });
}

TEST_CASE("Three active players with two contribution levels", "[side_pots]")
{
    const std::vector<PotPlayerState> players = {
        { 50, false },
        { 150, false },
        { 150, false },
    };

    const auto pots = compute_side_pots(players);

    REQUIRE(pots.size() == 2);
    REQUIRE(pots[0].amount == 150);
    REQUIRE(pots[0].eligible_player_indices == std::vector<size_t> { 0, 1, 2 });
    REQUIRE(pots[1].amount == 200);
    REQUIRE(pots[1].eligible_player_indices == std::vector<size_t> { 1, 2 });
}
