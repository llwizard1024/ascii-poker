#include <algorithm>
#include <catch_amalgamated.hpp>
#include <set>

#include "poker/card.h"
#include "poker/deck.h"

using namespace poker;

TEST_CASE("Card string representation", "[card]")
{
    Card card { Suit::Clubs, Rank::Ten };
    REQUIRE(card.to_string() == "10C");

    Card ace_spades { Suit::Spades, Rank::Ace };
    REQUIRE(ace_spades.to_string() == "AS");
}

TEST_CASE("Card comparison logic", "[card]")
{
    Card hearts_ace { Suit::Hearts, Rank::Ace };
    Card spades_king { Suit::Spades, Rank::King };

    REQUIRE(hearts_ace > spades_king);

    Card spades_ace { Suit::Spades, Rank::Ace };
    REQUIRE(spades_ace > hearts_ace);

    Card another_hearts_ace { Suit::Hearts, Rank::Ace };
    REQUIRE(hearts_ace == another_hearts_ace);
}

TEST_CASE("Deck initialization", "[deck]")
{
    Deck deck;

    REQUIRE(deck.size() == 52);
    REQUIRE_FALSE(deck.empty());

    std::set<Card> unique_cards;

    auto all_cards = deck.deal(52);
    for (const auto& card : all_cards) {
        unique_cards.insert(card);
    }

    REQUIRE(unique_cards.size() == 52);
}

TEST_CASE("Deck shuffling change order", "[deck]")
{
    Deck ordered_deck;
    Deck shuffled_deck;

    shuffled_deck.shuffle();

    auto ordered_cards = ordered_deck.deal(52);
    auto shuffled_cards = shuffled_deck.deal(52);

    bool is_different = std::equal(ordered_cards.begin(), ordered_cards.end(), shuffled_cards.begin()) == false;

    REQUIRE(is_different);
}

TEST_CASE("Deck dealing mechanics", "[deck]")
{
    Deck deck;

    int deal_count = 5;
    auto hand = deck.deal(deal_count);

    REQUIRE(hand.size() == 5);
    REQUIRE(deck.size() == 47);

    auto remaining_cards = deck.deal(47);

    for (const auto& dealt_card : hand) {
        auto it = std::find(remaining_cards.begin(), remaining_cards.end(), dealt_card);
        REQUIRE(it == remaining_cards.end());
    }
}

TEST_CASE("Deck bounds and exceptions", "[deck]")
{
    Deck deck;

    REQUIRE_THROWS_AS(deck.deal(53), std::out_of_range);

    deck.deal(52);
    REQUIRE(deck.empty());
    REQUIRE(deck.size() == 0);

    REQUIRE_THROWS_AS(deck.deal(1), std::out_of_range);
}