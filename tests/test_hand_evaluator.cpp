#include "poker/hand_evaluator.h"

#include <catch_amalgamated.hpp>

using namespace poker;

TEST_CASE("Straight flush beats four of a kind", "[hand_evaluator]")
{
    const HandValue straight_flush = evaluate_five({
        Card(Suit::Spades, Rank::Nine),
        Card(Suit::Spades, Rank::Ten),
        Card(Suit::Spades, Rank::Jack),
        Card(Suit::Spades, Rank::Queen),
        Card(Suit::Spades, Rank::King),
    });

    const HandValue four_kind = evaluate_five({
        Card(Suit::Hearts, Rank::Ace),
        Card(Suit::Diamonds, Rank::Ace),
        Card(Suit::Clubs, Rank::Ace),
        Card(Suit::Spades, Rank::Ace),
        Card(Suit::Hearts, Rank::Two),
    });

    REQUIRE(straight_flush > four_kind);
}

TEST_CASE("Wheel straight is detected", "[hand_evaluator]")
{
    const HandValue wheel = evaluate_five({
        Card(Suit::Hearts, Rank::Ace),
        Card(Suit::Clubs, Rank::Two),
        Card(Suit::Diamonds, Rank::Three),
        Card(Suit::Spades, Rank::Four),
        Card(Suit::Hearts, Rank::Five),
    });

    REQUIRE(wheel.category == HandCategory::Straight);
    REQUIRE(wheel.tiebreakers[0] == 5);
}

TEST_CASE("Best hand is chosen from seven cards", "[hand_evaluator]")
{
    const HandValue best = evaluate_best({
        Card(Suit::Hearts, Rank::Ace),
        Card(Suit::Spades, Rank::Ace),
        Card(Suit::Clubs, Rank::King),
        Card(Suit::Diamonds, Rank::King),
        Card(Suit::Hearts, Rank::Nine),
        Card(Suit::Spades, Rank::Seven),
        Card(Suit::Clubs, Rank::Two),
    });

    REQUIRE(best.category == HandCategory::TwoPair);
    REQUIRE(best.tiebreakers[0] == rank_value(Rank::Ace));
    REQUIRE(best.tiebreakers[1] == rank_value(Rank::King));
}
