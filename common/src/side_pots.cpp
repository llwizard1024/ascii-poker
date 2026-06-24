#include "poker/side_pots.h"

#include <algorithm>
#include <limits>

namespace poker {

std::vector<SidePot> compute_side_pots(const std::vector<PotPlayerState>& players)
{
    std::vector<uint32_t> remaining;
    remaining.reserve(players.size());
    for (const auto& player : players) {
        remaining.push_back(player.contributed);
    }

    std::vector<size_t> eligible;
    for (size_t i = 0; i < players.size(); ++i) {
        if (!players[i].folded) {
            eligible.push_back(i);
        }
    }

    std::vector<SidePot> pots;

    while (!eligible.empty()) {
        uint32_t min_bet = std::numeric_limits<uint32_t>::max();
        for (const size_t index : eligible) {
            if (remaining[index] > 0) {
                min_bet = std::min(min_bet, remaining[index]);
            }
        }

        if (min_bet == std::numeric_limits<uint32_t>::max()) {
            break;
        }

        SidePot pot;
        pot.amount = 0;
        for (size_t i = 0; i < remaining.size(); ++i) {
            if (remaining[i] == 0) {
                continue;
            }
            const uint32_t share = std::min(remaining[i], min_bet);
            pot.amount += share;
            remaining[i] -= share;
        }

        pot.eligible_player_indices = eligible;
        pots.push_back(pot);

        eligible.erase(
            std::remove_if(eligible.begin(), eligible.end(),
                [&](const size_t index) { return remaining[index] == 0; }),
            eligible.end());
    }

    return pots;
}

} // namespace poker
