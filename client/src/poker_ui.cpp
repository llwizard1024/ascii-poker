#include "poker_ui.h"
#include "application.h"
#include "poker/protocol.h"
#include "ftxui/ftxui_all.hpp"

#include <sstream>

using namespace ftxui;

namespace {
constexpr size_t kMaxLogMessages = 500;
}

static std::string format_server_message(const poker::protocol::ServerMessage& msg) {
    std::stringstream ss;
    std::visit([&](const auto& m) {
        using T = std::decay_t<decltype(m)>;
        if constexpr (std::is_same_v<T, poker::protocol::RoomList>) {
            ss << "=== Room List ===\n";
            if (m.rooms.empty()) {
                ss << "(no rooms)\n";
            }
            for (const auto& r : m.rooms) {
                ss << "Room " << r.room_id << ": " << r.room_name
                   << " [" << (int)r.current_players << "/" << (int)r.max_players << "]\n";
            }
        } else if constexpr (std::is_same_v<T, poker::protocol::JoinedRoom>) {
            ss << "Joined Room " << m.room_id << "\nPlayers:";
            for (const auto& name : m.player_names) ss << " " << name;
            ss << "\n";
        } else if constexpr (std::is_same_v<T, poker::protocol::GameStateUpdate>) {
            ss << "--- Game State ---\n";
            ss << "Phase: ";
            switch (m.phase) {
                case poker::protocol::GamePhase::PreFlop: ss << "Pre-Flop"; break;
                case poker::protocol::GamePhase::Flop: ss << "Flop"; break;
                case poker::protocol::GamePhase::Turn: ss << "Turn"; break;
                case poker::protocol::GamePhase::River: ss << "River"; break;
                case poker::protocol::GamePhase::Showdown: ss << "Showdown"; break;
            }
            ss << "\nPot: " << m.total_pot;
            ss << "\nYour cards:";
            for (const auto& c : m.your_cards) ss << " " << c.to_string();
            ss << "\nCommunity cards:";
            for (const auto& c : m.general_cards) ss << " " << c.to_string();
            if (!m.active_player_name.empty())
                ss << "\nActive player: " << m.active_player_name;
            ss << "\n";
        } else if constexpr (std::is_same_v<T, poker::protocol::YourTurn>) {
            ss << "*** YOUR TURN ***\n";
            ss << "Possible actions:";
            for (const auto& a : m.available_actions) {
                switch (a) {
                    case poker::protocol::Action::Fold: ss << " Fold"; break;
                    case poker::protocol::Action::Check: ss << " Check"; break;
                    case poker::protocol::Action::Call: ss << " Call"; break;
                    case poker::protocol::Action::Raise: ss << " Raise"; break;
                }
            }
            ss << "\nBet range: " << m.min_amount << " - " << m.max_amount << "\n";
        } else if constexpr (std::is_same_v<T, poker::protocol::HandResult>) {
            ss << "=== Hand Result ===\n";
            ss << "Winner(s):";
            for (const auto& name : m.winner_names) ss << " " << name;
            ss << "\nPot: " << m.pot_amount << "\n";
        } else if constexpr (std::is_same_v<T, poker::protocol::LeftRoom>) {
            ss << "Left room " << m.room_id << "\n";
        } else if constexpr (std::is_same_v<T, poker::protocol::Error>) {
            ss << "Error (" << m.code << "): " << m.description << "\n";
        }
    }, msg);
    return ss.str();
}

void PokerUI::add_server_message(const poker::protocol::ServerMessage& msg) {
    add_message(format_server_message(msg));
}

PokerUI::PokerUI(std::shared_ptr<ClientApplication> app)
    : app_(std::move(app)) {}

void PokerUI::set_screen(ftxui::ScreenInteractive& screen) {
    screen_ = &screen;
}

void PokerUI::add_message(const std::string& msg) {
    {
        std::lock_guard lock(mtx_);
        messages_.push_back(msg);
        while (messages_.size() > kMaxLogMessages) {
            messages_.pop_front();
        }
    }
    if (screen_) {
        screen_->Post(ftxui::Event::Custom);
    }
}

void PokerUI::run() {
    auto screen = ScreenInteractive::Fullscreen();
    set_screen(screen);

    std::string input;
    auto input_component = Input(&input, "enter command");

    input_component |= CatchEvent([&](Event event) {
        if (event == Event::Return) {
            if (!input.empty()) {
                app_->process_input(input);
                if (app_->quit_requested()) {
                    screen.Exit();
                }
                input = "";
            }
            return true;
        }
        return false;
    });

    auto renderer = Renderer(input_component, [&] {
        std::lock_guard lock(mtx_);
        Elements log_elements;
        for (const auto& msg : messages_) {
            log_elements.push_back(text(msg));
        }
        auto log_panel = vbox(std::move(log_elements)) | border | flex;
        return vbox(
            log_panel | flex_grow,
            separator(),
            hbox(text("> "), input_component->Render())
        ) | border;
    });

    screen.Loop(renderer);
}
