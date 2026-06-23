#include "client_view_state.h"

#include "poker/protocol.h"

#include <sstream>
#include <type_traits>
#include <variant>

namespace poker::client {


namespace {
constexpr size_t kMaxLogLines = 200;
}

void ClientViewState::append_log(std::string message)
{
    log.push_back(std::move(message));
    while (log.size() > kMaxLogLines) {
        log.pop_front();
    }
}

std::string phase_to_string(poker::protocol::GamePhase phase)
{
    switch (phase) {
    case poker::protocol::GamePhase::PreFlop:
        return "Pre-Flop";
    case poker::protocol::GamePhase::Flop:
        return "Flop";
    case poker::protocol::GamePhase::Turn:
        return "Turn";
    case poker::protocol::GamePhase::River:
        return "River";
    case poker::protocol::GamePhase::Showdown:
        return "Showdown";
    }
    return "Unknown";
}

std::string action_to_string(poker::protocol::Action action)
{
    switch (action) {
    case poker::protocol::Action::Fold:
        return "Fold";
    case poker::protocol::Action::Check:
        return "Check";
    case poker::protocol::Action::Call:
        return "Call";
    case poker::protocol::Action::Raise:
        return "Raise";
    }
    return "?";
}

void ClientViewState::apply_message(const poker::protocol::ServerMessage& msg)
{
    std::visit([this](const auto& concrete) {
        using T = std::decay_t<decltype(concrete)>;

        if constexpr (std::is_same_v<T, poker::protocol::Welcome>) {
            player_name = concrete.player_name;
            screen = UiScreen::Lobby;
            append_log("Welcome, " + player_name);
            status_message = "Welcome, " + player_name;
        } else if constexpr (std::is_same_v<T, poker::protocol::RoomList>) {
            rooms = concrete.rooms;
            append_log("Room list updated (" + std::to_string(rooms.size()) + " rooms)");
        } else if constexpr (std::is_same_v<T, poker::protocol::JoinedRoom>) {
            room_id = concrete.room_id;
            player_names = concrete.player_names;
            room_host_name = concrete.host_name;
            room_max_players = concrete.max_players;
            is_room_host = !player_name.empty() && player_name == concrete.host_name;
            game_state.reset();
            your_turn.reset();
            last_hand_result.reset();
            screen = UiScreen::WaitingRoom;
            std::ostringstream ss;
            ss << "Joined room " << concrete.room_id << " (";
            for (size_t i = 0; i < player_names.size(); ++i) {
                if (i > 0) {
                    ss << ", ";
                }
                ss << player_names[i];
            }
            ss << ")";
            append_log(ss.str());
            if (is_room_host) {
                status_message = "You are the host — start when ready";
            } else {
                status_message = "Waiting for " + concrete.host_name + " to start";
            }
        } else if constexpr (std::is_same_v<T, poker::protocol::GameStateUpdate>) {
            game_state = concrete;
            screen = UiScreen::InGame;
            status_message = phase_to_string(concrete.phase);
            if (!concrete.active_player_name.empty()) {
                status_message += " — " + concrete.active_player_name + " to act";
            }
        } else if constexpr (std::is_same_v<T, poker::protocol::YourTurn>) {
            your_turn = concrete;
            screen = UiScreen::InGame;
            status_message = "Your turn!";
        } else if constexpr (std::is_same_v<T, poker::protocol::HandResult>) {
            last_hand_result = concrete;
            std::ostringstream ss;
            ss << "Hand won by";
            for (const auto& name : concrete.winner_names) {
                ss << " " << name;
            }
            ss << " (" << concrete.pot_amount << " chips)";
            append_log(ss.str());
            status_message = ss.str();
        } else if constexpr (std::is_same_v<T, poker::protocol::LeftRoom>) {
            room_id.reset();
            player_names.clear();
            room_host_name.clear();
            room_max_players = 0;
            is_room_host = false;
            game_state.reset();
            your_turn.reset();
            screen = UiScreen::Lobby;
            append_log("Left room " + std::to_string(concrete.room_id));
            status_message = "In lobby";
        } else if constexpr (std::is_same_v<T, poker::protocol::Error>) {
            append_log("Error: " + concrete.description);
            status_message = concrete.description;
        }
    },
        msg);
}

} // namespace poker::client
