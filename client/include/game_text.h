#pragma once

#include "poker/card.h"
#include "poker/protocol.h"

#include <string>
#include <vector>

namespace poker::client {

std::string describe_hand(const std::vector<poker::Card>& hole_cards, const std::vector<poker::Card>& board);
std::string format_action_event(const poker::protocol::ActionEvent& event);

} // namespace poker::client
