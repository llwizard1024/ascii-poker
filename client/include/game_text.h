#pragma once

#include "poker/card.h"
#include "poker/protocol.h"

#include <string>
#include <vector>

namespace poker::client {

std::string describe_hand(const std::vector<poker::Card>& hole_cards, const std::vector<poker::Card>& board);
std::string describe_starting_hand(const std::vector<poker::Card>& hole_cards);
std::string format_action_event(const poker::protocol::ActionEvent& event);
std::string winner_hand_label(
    const std::string& winner_name,
    const poker::protocol::GameStateUpdate& game_state);

} // namespace poker::client
