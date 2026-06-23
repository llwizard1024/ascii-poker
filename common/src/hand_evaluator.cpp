#include "poker/hand_evaluator.h"

#include <algorithm>
#include <array>

namespace poker {
namespace {

    bool is_straight(const std::array<uint8_t, 5>& values, uint8_t& high)
    {
        std::array<uint8_t, 5> sorted = values;
        std::sort(sorted.begin(), sorted.end());

        bool normal = true;
        for (size_t i = 1; i < 5; ++i) {
            if (sorted[i] != sorted[i - 1] + 1) {
                normal = false;
                break;
            }
        }
        if (normal) {
            high = sorted[4];
            return true;
        }

        if (sorted[0] == 2 && sorted[1] == 3 && sorted[2] == 4 && sorted[3] == 5 && sorted[4] == 14) {
            high = 5;
            return true;
        }

        return false;
    }

    HandValue make_hand(HandCategory category, std::array<uint8_t, 5> tiebreakers)
    {
        HandValue result;
        result.category = category;
        result.tiebreakers = tiebreakers;
        return result;
    }

    HandValue evaluate_five_values(std::array<uint8_t, 5> values, bool flush)
    {
        std::sort(values.begin(), values.end(), std::greater<uint8_t>());

        std::array<int, 15> counts {};
        for (uint8_t v : values) {
            ++counts[v];
        }

        uint8_t straight_high = 0;
        const bool straight = is_straight(values, straight_high);

        if (flush && straight) {
            return make_hand(HandCategory::StraightFlush, { straight_high, 0, 0, 0, 0 });
        }

        for (int v = 14; v >= 2; --v) {
            if (counts[v] == 4) {
                uint8_t kicker = 0;
                for (int k = 14; k >= 2; --k) {
                    if (counts[k] == 1) {
                        kicker = static_cast<uint8_t>(k);
                        break;
                    }
                }
                return make_hand(HandCategory::FourOfKind, { static_cast<uint8_t>(v), kicker, 0, 0, 0 });
            }
        }

        uint8_t trip = 0;
        uint8_t pair = 0;
        for (int v = 14; v >= 2; --v) {
            if (counts[v] == 3 && trip == 0) {
                trip = static_cast<uint8_t>(v);
            } else if (counts[v] == 2 && pair == 0) {
                pair = static_cast<uint8_t>(v);
            }
        }
        if (trip && pair) {
            return make_hand(HandCategory::FullHouse, { trip, pair, 0, 0, 0 });
        }

        if (flush) {
            return make_hand(HandCategory::Flush, values);
        }

        if (straight) {
            return make_hand(HandCategory::Straight, { straight_high, 0, 0, 0, 0 });
        }

        if (trip) {
            std::array<uint8_t, 2> kickers {};
            size_t ki = 0;
            for (uint8_t v : values) {
                if (v != trip && ki < 2) {
                    kickers[ki++] = v;
                }
            }
            return make_hand(HandCategory::ThreeOfKind, { trip, kickers[0], kickers[1], 0, 0 });
        }

        std::array<uint8_t, 2> pairs {};
        size_t pi = 0;
        for (int v = 14; v >= 2; --v) {
            if (counts[v] == 2 && pi < 2) {
                pairs[pi++] = static_cast<uint8_t>(v);
            }
        }
        if (pi == 2) {
            uint8_t kicker = 0;
            for (uint8_t v : values) {
                if (v != pairs[0] && v != pairs[1]) {
                    kicker = v;
                    break;
                }
            }
            return make_hand(HandCategory::TwoPair, { pairs[0], pairs[1], kicker, 0, 0 });
        }

        if (pi == 1) {
            std::array<uint8_t, 3> kickers {};
            size_t ki = 0;
            for (uint8_t v : values) {
                if (v != pairs[0] && ki < 3) {
                    kickers[ki++] = v;
                }
            }
            return make_hand(HandCategory::OnePair, { pairs[0], kickers[0], kickers[1], kickers[2], 0 });
        }

        return make_hand(HandCategory::HighCard, values);
    }

} // namespace

HandValue evaluate_five(const std::array<Card, 5>& cards)
{
    std::array<uint8_t, 5> values {};
    for (size_t i = 0; i < 5; ++i) {
        values[i] = rank_value(cards[i].rank());
    }

    const Suit suit = cards[0].suit();
    const bool flush = std::all_of(cards.begin(), cards.end(), [suit](const Card& c) {
        return c.suit() == suit;
    });

    return evaluate_five_values(values, flush);
}

HandValue evaluate_best(const std::vector<Card>& cards)
{
    if (cards.size() < 5) {
        return {};
    }

    HandValue best;
    const size_t n = cards.size();
    std::array<size_t, 5> idx { 0, 1, 2, 3, 4 };

    auto next_combination = [n](std::array<size_t, 5>& c) -> bool {
        for (int i = 4; i >= 0; --i) {
            if (c[static_cast<size_t>(i)] != static_cast<size_t>(i) + n - 5) {
                ++c[static_cast<size_t>(i)];
                for (size_t j = static_cast<size_t>(i) + 1; j < 5; ++j) {
                    c[j] = c[j - 1] + 1;
                }
                return true;
            }
        }
        return false;
    };

    do {
        std::array<Card, 5> hand {
            cards[idx[0]],
            cards[idx[1]],
            cards[idx[2]],
            cards[idx[3]],
            cards[idx[4]],
        };
        const HandValue value = evaluate_five(hand);
        if (value > best) {
            best = value;
        }
    } while (next_combination(idx));

    return best;
}

} // namespace poker
