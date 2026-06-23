#pragma once

#include <cstdint>
#include <vector>

namespace poker {

struct PotPlayerState {
    uint32_t contributed = 0;
    bool folded = false;
};

struct SidePot {
    uint32_t amount = 0;
    std::vector<size_t> eligible_player_indices;
};

std::vector<SidePot> compute_side_pots(const std::vector<PotPlayerState>& players);

} // namespace poker
