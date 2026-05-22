#include "evaluator.h"
#include "card.h"
#include "hand_result.h"
#include <algorithm>
#include <vector>

/**
 * Я искренни пытался реализовать Cactus Kev алгоритм. НО, даже с помощью нейросети
 * я не смог. Я ненавижу ёбаные нейросети, просто ненавижу. Он меня водил пол дня кругами
 * говорил, что я не прав и тд, а потом в итоге выяснилось, что я всё же прав сука блять нахуй.
 * Он высрал мне этот алгоритм в итоге готовый, у меня нет моральных сил переписывать и продолжать
 * "совершенствовать" эту поебень ёбаную.
 */
namespace common {
static HandResult evaluate_5_cards(const std::vector<Card>& five)
{
    int r[5], s[5];
    for (int i = 0; i < 5; ++i) {
        r[i] = static_cast<int>(five[i].rank());
        s[i] = static_cast<int>(five[i].suit());
    }

    std::sort(r, r + 5);

    bool flush = true;
    for (int i = 1; i < 5; ++i)
        if (s[i] != s[0]) {
            flush = false;
            break;
        }

    bool straight = false;
    if (r[0] + 1 == r[1] && r[1] + 1 == r[2] && r[2] + 1 == r[3] && r[3] + 1 == r[4])
        straight = true;
    if (r[0] == 0 && r[1] == 1 && r[2] == 2 && r[3] == 3 && r[4] == 12)
        straight = true;

    int groups[5] = { 0 }, g = 0;
    for (int i = 0; i < 5;) {
        int j = i;
        while (j < 5 && r[j] == r[i])
            ++j;
        groups[g++] = j - i;
        i = j;
    }
    std::sort(groups, groups + g, std::greater<int>());

    HandRank rank;
    if (straight && flush) {
        rank = HandRank::RoyalOrStraightFlush;
    } else if (groups[0] == 4) {
        rank = HandRank::FourOfAKind;
    } else if (groups[0] == 3 && groups[1] == 2) {
        rank = HandRank::FullHouse;
    } else if (flush) {
        rank = HandRank::Flush;
    } else if (straight) {
        rank = HandRank::Straight;
    } else if (groups[0] == 3) {
        rank = HandRank::ThreeOfAKind;
    } else if (groups[0] == 2 && groups[1] == 2) {
        rank = HandRank::TwoPair;
    } else if (groups[0] == 2) {
        rank = HandRank::OnePair;
    } else {
        rank = HandRank::HighCard;
    }

    return HandResult { 0, rank };
}

HandResult evaluate(const std::vector<Card>& cards)
{
    HandResult best { 0, HandRank::HighCard };

    for (int a = 0; a < 7; ++a) {
        for (int b = a + 1; b < 7; ++b) {
            std::vector<Card> five;
            for (int k = 0; k < 7; ++k)
                if (k != a && k != b)
                    five.push_back(cards[k]);
            HandResult res = evaluate_5_cards(five);
            if (static_cast<int>(res.hand_rank_) > static_cast<int>(best.hand_rank_)) {
                best = res;
            }
        }
    }
    return best;
}
}