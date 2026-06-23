#pragma once

#include "poker/card.h"

#include <array>
#include <cstdint>
#include <vector>

namespace poker {

enum class HandCategory : uint8_t {
    HighCard = 0,
    OnePair = 1,
    TwoPair = 2,
    ThreeOfKind = 3,
    Straight = 4,
    Flush = 5,
    FullHouse = 6,
    FourOfKind = 7,
    StraightFlush = 8,
};

struct HandValue {
    HandCategory category = HandCategory::HighCard;
    std::array<uint8_t, 5> tiebreakers {};

    friend auto operator<=>(const HandValue&, const HandValue&) = default;
};

inline uint8_t rank_value(Rank rank)
{
    return static_cast<uint8_t>(rank) + 2;
}

HandValue evaluate_five(const std::array<Card, 5>& cards);
HandValue evaluate_best(const std::vector<Card>& cards);

} // namespace poker
