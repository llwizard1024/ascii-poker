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

    uint8_t rank_order(poker::Rank rank)
    {
        switch (rank) {
        case poker::Rank::Two:
            return 2;
        case poker::Rank::Three:
            return 3;
        case poker::Rank::Four:
            return 4;
        case poker::Rank::Five:
            return 5;
        case poker::Rank::Six:
            return 6;
        case poker::Rank::Seven:
            return 7;
        case poker::Rank::Eight:
            return 8;
        case poker::Rank::Nine:
            return 9;
        case poker::Rank::Ten:
            return 10;
        case poker::Rank::Jack:
            return 11;
        case poker::Rank::Queen:
            return 12;
        case poker::Rank::King:
            return 13;
        case poker::Rank::Ace:
            return 14;
        }
        return 0;
    }

    std::string rank_short(poker::Rank rank)
    {
        switch (rank) {
        case poker::Rank::Ten:
            return "T";
        case poker::Rank::Jack:
            return "J";
        case poker::Rank::Queen:
            return "Q";
        case poker::Rank::King:
            return "K";
        case poker::Rank::Ace:
            return "A";
        default:
            return std::to_string(rank_order(rank));
        }
    }

    std::string hole_notation(const poker::Card& high, const poker::Card& low)
    {
        const std::string ranks = rank_short(high.rank()) + rank_short(low.rank());
        if (high.suit() == low.suit()) {
            return ranks + "s";
        }
        return ranks + "o";
    }

} // namespace

std::string describe_hand(const std::vector<poker::Card>& hole_cards, const std::vector<poker::Card>& board)
{
    if (hole_cards.size() < 2) {
        return {};
    }

    if (board.empty()) {
        return describe_starting_hand(hole_cards);
    }

    std::vector<poker::Card> cards = hole_cards;
    cards.insert(cards.end(), board.begin(), board.end());
    if (cards.size() < 5) {
        return tr(Msg::WaitingForBoard);
    }

    return tr_hand_category(poker::evaluate_best(cards).category);
}

std::string describe_starting_hand(const std::vector<poker::Card>& hole_cards)
{
    if (hole_cards.size() < 2) {
        return {};
    }

    poker::Card high = hole_cards[0];
    poker::Card low = hole_cards[1];
    if (rank_order(low.rank()) > rank_order(high.rank())) {
        std::swap(high, low);
    }

    if (high.rank() == low.rank()) {
        return tr(Msg::PreflopPocketPair, rank_short(high.rank()));
    }

    const std::string notation = hole_notation(high, low);
    if (high.suit() == low.suit()) {
        return tr(Msg::PreflopSuited, notation);
    }

    if (rank_order(high.rank()) >= 10 && rank_order(low.rank()) >= 10) {
        return tr(Msg::PreflopBroadway);
    }

    if (rank_order(high.rank()) - rank_order(low.rank()) == 1) {
        return tr(Msg::PreflopConnector);
    }

    if (high.rank() == poker::Rank::Ace || low.rank() == poker::Rank::Ace) {
        return tr(Msg::PreflopAce);
    }

    return tr(Msg::PreflopHighCard);
}

std::string winner_hand_label(
    const std::string& winner_name,
    const poker::protocol::GameStateUpdate& game_state)
{
    for (const auto& player : game_state.players) {
        if (player.name != winner_name || player.hole_cards.size() < 2) {
            continue;
        }
        std::vector<poker::Card> cards = player.hole_cards;
        cards.insert(cards.end(), game_state.general_cards.begin(), game_state.general_cards.end());
        if (cards.size() < 5) {
            return {};
        }
        return tr_hand_category(poker::evaluate_best(cards).category);
    }
    return {};
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
