#include "card.h"
#include "evaluator.h"
#include "hand_result.h"
#include <catch2/catch_amalgamated.hpp>
#include <vector>

using namespace common;

Card C(Rank r, Suit s) { return Card(r, s); }

TEST_CASE("Royal Flush", "[evaluator]")
{
    std::vector<Card> hand = {
        C(Rank::Ace, Suit::Spades), C(Rank::King, Suit::Spades),
        C(Rank::Queen, Suit::Spades), C(Rank::Jack, Suit::Spades),
        C(Rank::Ten, Suit::Spades),
        C(Rank::Two, Suit::Hearts),
        C(Rank::Three, Suit::Diamonds)
    };
    HandResult res = evaluate(hand);
    REQUIRE(res.hand_rank_ == HandRank::RoyalOrStraightFlush);
}

TEST_CASE("Straight Flush", "[evaluator]")
{
    std::vector<Card> hand = {
        C(Rank::Nine, Suit::Hearts), C(Rank::Eight, Suit::Hearts),
        C(Rank::Seven, Suit::Hearts), C(Rank::Six, Suit::Hearts),
        C(Rank::Five, Suit::Hearts),
        C(Rank::Ace, Suit::Spades),
        C(Rank::King, Suit::Diamonds)
    };
    HandResult res = evaluate(hand);
    REQUIRE(res.hand_rank_ == HandRank::RoyalOrStraightFlush);
}

TEST_CASE("Four of a Kind", "[evaluator]")
{
    std::vector<Card> hand = {
        C(Rank::Ace, Suit::Spades), C(Rank::Ace, Suit::Hearts),
        C(Rank::Ace, Suit::Diamonds), C(Rank::Ace, Suit::Clubs),
        C(Rank::King, Suit::Spades),
        C(Rank::Queen, Suit::Hearts),
        C(Rank::Jack, Suit::Diamonds)
    };
    HandResult res = evaluate(hand);
    REQUIRE(res.hand_rank_ == HandRank::FourOfAKind);
}

TEST_CASE("Full House", "[evaluator]")
{
    std::vector<Card> hand = {
        C(Rank::Ace, Suit::Spades), C(Rank::Ace, Suit::Hearts),
        C(Rank::Ace, Suit::Diamonds),
        C(Rank::King, Suit::Spades), C(Rank::King, Suit::Hearts),
        C(Rank::Two, Suit::Clubs),
        C(Rank::Three, Suit::Diamonds)
    };
    HandResult res = evaluate(hand);
    REQUIRE(res.hand_rank_ == HandRank::FullHouse);
}

TEST_CASE("Flush", "[evaluator]")
{
    std::vector<Card> hand = {
        C(Rank::Ace, Suit::Clubs), C(Rank::Ten, Suit::Clubs),
        C(Rank::Seven, Suit::Clubs), C(Rank::Four, Suit::Clubs),
        C(Rank::Two, Suit::Clubs),
        C(Rank::King, Suit::Hearts),
        C(Rank::Queen, Suit::Diamonds)
    };
    HandResult res = evaluate(hand);
    REQUIRE(res.hand_rank_ == HandRank::Flush);
}

TEST_CASE("Straight", "[evaluator]")
{
    std::vector<Card> hand = {
        C(Rank::Nine, Suit::Spades), C(Rank::Eight, Suit::Hearts),
        C(Rank::Seven, Suit::Diamonds), C(Rank::Six, Suit::Clubs),
        C(Rank::Five, Suit::Spades),
        C(Rank::Ace, Suit::Hearts),
        C(Rank::King, Suit::Diamonds)
    };
    HandResult res = evaluate(hand);
    REQUIRE(res.hand_rank_ == HandRank::Straight);
}

TEST_CASE("Three of a Kind", "[evaluator]")
{
    std::vector<Card> hand = {
        C(Rank::Queen, Suit::Spades), C(Rank::Queen, Suit::Hearts),
        C(Rank::Queen, Suit::Diamonds),
        C(Rank::Ace, Suit::Clubs), C(Rank::King, Suit::Spades),
        C(Rank::Two, Suit::Hearts),
        C(Rank::Three, Suit::Diamonds)
    };
    HandResult res = evaluate(hand);
    REQUIRE(res.hand_rank_ == HandRank::ThreeOfAKind);
}

TEST_CASE("Two Pair", "[evaluator]")
{
    std::vector<Card> hand = {
        C(Rank::Ace, Suit::Spades), C(Rank::Ace, Suit::Hearts),
        C(Rank::King, Suit::Diamonds), C(Rank::King, Suit::Clubs),
        C(Rank::Queen, Suit::Spades),
        C(Rank::Two, Suit::Hearts),
        C(Rank::Three, Suit::Diamonds)
    };
    HandResult res = evaluate(hand);
    REQUIRE(res.hand_rank_ == HandRank::TwoPair);
}

TEST_CASE("One Pair", "[evaluator]")
{
    std::vector<Card> hand = {
        C(Rank::Ace, Suit::Spades), C(Rank::Ace, Suit::Hearts),
        C(Rank::King, Suit::Diamonds), C(Rank::Queen, Suit::Clubs),
        C(Rank::Jack, Suit::Spades),
        C(Rank::Two, Suit::Hearts),
        C(Rank::Three, Suit::Diamonds)
    };
    HandResult res = evaluate(hand);
    REQUIRE(res.hand_rank_ == HandRank::OnePair);
}

TEST_CASE("High Card", "[evaluator]")
{
    std::vector<Card> hand = {
        C(Rank::Ace, Suit::Spades), C(Rank::King, Suit::Hearts),
        C(Rank::Queen, Suit::Diamonds), C(Rank::Jack, Suit::Clubs),
        C(Rank::Nine, Suit::Spades),
        C(Rank::Two, Suit::Hearts),
        C(Rank::Three, Suit::Diamonds)
    };
    HandResult res = evaluate(hand);
    REQUIRE(res.hand_rank_ == HandRank::HighCard);
}