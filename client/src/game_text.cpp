#include "game_text.h"

#include "i18n.h"

#include <poker/hand_evaluator.h>

#include <cctype>
#include <sstream>

namespace poker::client {

namespace {

std::string trim_copy(std::string value)
{
    const auto start = value.find_first_not_of(" \t");
    if (start == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t");
    return value.substr(start, end - start + 1);
}

} // namespace

std::string describe_hand(const std::vector<poker::Card>& hole_cards, const std::vector<poker::Card>& board)
{
    if (hole_cards.size() < 2) {
        return {};
    }

    std::vector<poker::Card> cards = hole_cards;
    cards.insert(cards.end(), board.begin(), board.end());
    if (cards.size() < 5) {
        return tr(Msg::WaitingForBoard);
    }

    return tr_hand_category(poker::evaluate_best(cards).category);
}

std::string format_action_event(const poker::protocol::ActionEvent& event)
{
    std::ostringstream ss;
    ss << event.player_name << " " << trim_copy(tr_action(event.action));
    if (event.amount.has_value()) {
        ss << " " << *event.amount;
    }
    return ss.str();
}

} // namespace poker::client
