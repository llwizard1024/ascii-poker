#pragma once
#include <cstdint>

namespace common {

enum class HandRank : uint8_t {
    HighCard = 0,
    OnePair = 1,
    TwoPair = 2,
    ThreeOfAKind = 3,
    Straight = 4,
    Flush = 5,
    FullHouse = 6,
    FourOfAKind = 7,
    RoyalOrStraightFlush = 8
};

class HandResult {
public:
    int value_;
    HandRank hand_rank_;

    HandResult(int value, HandRank hand_rank)
        : value_(value)
        , hand_rank_(hand_rank)
    {
    }
};
}