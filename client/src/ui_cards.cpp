#include "ui_cards.h"

#include "client_view_state.h"
#include "i18n.h"
#include "ui_theme.h"

#include <ftxui_all.hpp>

#include <algorithm>
#include <sstream>

namespace poker::client {

using namespace ftxui;

namespace {

    std::string card_label(const poker::Card& card)
    {
        const std::string raw = card.to_string();
        const char suit = raw.back();
        std::string rank = raw.substr(0, raw.size() - 1);
        std::string sym;
        switch (suit) {
        case 'H':
            sym = "\u2665";
            break;
        case 'D':
            sym = "\u2666";
            break;
        case 'C':
            sym = "\u2663";
            break;
        case 'S':
            sym = "\u2660";
            break;
        default:
            sym = std::string(1, suit);
            break;
        }

        while (rank.size() < 2) {
            rank = " " + rank;
        }
        return rank + sym;
    }

    bool is_red_suit(char suit)
    {
        return suit == 'H' || suit == 'D';
    }

    Element styled_card_body(const std::string& label, bool red)
    {
        const std::string top = "+" + std::string(label.size() + 2, '-');
        const std::string mid = "| " + label + " |";
        const Color suit_color = red ? ui_theme::red_suit() : ui_theme::black_suit();
        return vbox({ text(top), text(mid), text(top) })
            | color(suit_color)
            | bgcolor(ui_theme::card_background())
            | border;
    }

    std::string player_role_suffix(const poker::protocol::PlayerState& player)
    {
        std::string suffix;
        if (player.is_dealer) {
            suffix += tr(Msg::PlayerDealer);
        }
        if (player.is_small_blind) {
            suffix += tr(Msg::PlayerSmallBlind);
        }
        if (player.is_big_blind) {
            suffix += tr(Msg::PlayerBigBlind);
        }
        return suffix;
    }

    std::string player_status_suffix(const poker::protocol::PlayerState& player)
    {
        if (player.folded) {
            return tr(Msg::PlayerFolded);
        }
        if (player.all_in) {
            return tr(Msg::PlayerAllIn);
        }
        return {};
    }

} // namespace

Element render_card(const poker::Card& card)
{
    const std::string label = card_label(card);
    return styled_card_body(label, is_red_suit(card.to_string().back()));
}

Element render_card_back()
{
    return vbox({
               text("+---+"),
               text("|## |"),
               text("+---+"),
           })
        | color(Color::RGB(120, 130, 145))
        | bgcolor(Color::RGB(30, 34, 44))
        | border;
}

Element render_card_row(const std::vector<poker::Card>& cards, const size_t slots)
{
    Elements elements;
    for (size_t i = 0; i < slots; ++i) {
        if (i < cards.size()) {
            elements.push_back(render_card(cards[i]));
        } else {
            elements.push_back(render_card_back());
        }
        elements.push_back(text(" "));
    }
    return hbox(std::move(elements));
}

Element render_table_seats(
    const std::vector<poker::protocol::PlayerState>& players,
    const std::string& self_name,
    const std::string& active_name)
{
    if (players.empty()) {
        return text(tr(Msg::NoPlayers)) | dim;
    }

    const auto render_seat = [&](const poker::protocol::PlayerState& player) {
        const bool active = player.name == active_name;
        const bool self = player.name == self_name;
        const bool folded = player.folded;
        std::ostringstream line;
        line << (active ? ">> " : "   ") << player.name;
        if (self) {
            line << tr(Msg::PlayerYou);
        }
        line << "  " << player.chips << tr(Msg::PlayerChips);
        if (player.round_bet > 0) {
            line << tr(Msg::PlayerBet) << player.round_bet;
        }
        line << player_role_suffix(player);
        line << player_status_suffix(player);

        Element seat = text(line.str());
        if (active) {
            seat = seat | color(Color::RGB(240, 210, 120)) | bold;
        } else if (folded) {
            seat = seat | dim;
        } else if (self) {
            seat = seat | color(Color::RGB(170, 200, 230));
        }
        if (!player.hole_cards.empty()) {
            seat = vbox({ seat, render_card_row(player.hole_cards, 2) });
        }
        return seat;
    };

    if (players.size() <= 3) {
        Elements rows;
        for (const auto& player : players) {
            rows.push_back(render_seat(player));
        }
        return vbox(std::move(rows));
    }

    const size_t top_count = (players.size() + 1) / 2;
    Elements top;
    Elements bottom;
    for (size_t i = 0; i < players.size(); ++i) {
        auto seat = render_seat(players[i]) | center;
        (i < top_count ? top : bottom).push_back(seat);
    }

    return vbox({
        hbox(std::move(top)) | center,
        text("  --- pot area ---  ") | dim | center,
        hbox(std::move(bottom)) | center,
    });
}

} // namespace poker::client
