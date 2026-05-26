#pragma once
#include <cstddef>
#include <cstdint>

enum class Suit : uint8_t {
    Clubs = 0,
    Diamonds = 1,
    Hearts = 2,
    Spades = 3
};

enum class Rank : uint8_t {
    Two = 0,
    Three = 1,
    Four = 2,
    Five = 3,
    Six = 4,
    Seven = 5,
    Eight = 6,
    Nine = 7,
    Ten = 8,
    Jack = 9,
    Queen = 10,
    King = 11,
    Ace = 12
};

class Card {
public:
    constexpr Card(Rank r, Suit s)
        : rank_ { r }
        , suit_ { s }
    {
    }

    Rank rank() const { return rank_; }
    Suit suit() const { return suit_; }

    int prime() const
    {
        return PRIMES_BY_RANK[static_cast<size_t>(rank_)];
    }

    int rankBitmask() const
    {
        if (rank_ == Rank::Ace) {
            return (1 << 12) | (1 << 0);
        }

        return 1 << static_cast<int>(rank_);
    }

private:
    Rank rank_;
    Suit suit_;

    static constexpr int PRIMES_BY_RANK[13] = {
        2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41
    };
};