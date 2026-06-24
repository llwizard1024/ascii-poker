#pragma once

#include "poker/card.h"
#include "poker/protocol.h"

#include <ftxui_all.hpp>

#include <string>
#include <vector>

namespace poker::client {

ftxui::Element render_card(const poker::Card& card);
ftxui::Element render_card_back();
ftxui::Element render_card_row(const std::vector<poker::Card>& cards, size_t slots);
ftxui::Element render_table_seats(
    const std::vector<poker::protocol::PlayerState>& players,
    const std::string& self_name,
    const std::string& active_name);

} // namespace poker::client
