#include "client_view_state.h"

#include "game_text.h"
#include "i18n.h"

#include <poker/protocol.h>

#include <algorithm>
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
    return tr_phase(phase);
}

std::string action_to_string(poker::protocol::Action action)
{
    return tr_action(action);
}

std::string connection_status_label(ConnectionStatus status)
{
    switch (status) {
    case ConnectionStatus::Connecting:
        return tr(Msg::ConnConnecting);
    case ConnectionStatus::Connected:
        return tr(Msg::ConnConnected);
    case ConnectionStatus::Failed:
        return tr(Msg::ConnFailed);
    case ConnectionStatus::Disconnected:
        return tr(Msg::ConnDisconnected);
    }
    return "?";
}

void ClientViewState::set_connection(ConnectionStatus status)
{
    connection = status;
    const std::string endpoint = server_host + ":" + server_port;
    if (status == ConnectionStatus::Connected) {
        status_message = tr(Msg::ConnectedTo, endpoint);
    } else if (status == ConnectionStatus::Connecting) {
        status_message = tr(Msg::ConnectingTo, endpoint);
    } else if (status == ConnectionStatus::Failed) {
        status_message = tr(Msg::CouldNotConnect, endpoint);
        screen = UiScreen::Login;
    } else if (status == ConnectionStatus::Disconnected) {
        status_message = tr(Msg::ConnectionLostShort);
        screen = UiScreen::Disconnected;
        your_turn.reset();
        reset_play_session();
    }
}

void ClientViewState::reset_play_session()
{
    room_id.reset();
    player_names.clear();
    room_host_name.clear();
    room_max_players = 0;
    is_room_host = false;
    game_state.reset();
    your_turn.reset();
    last_hand_result.reset();
}

bool ClientViewState::has_action(const poker::protocol::Action action) const
{
    if (!your_turn.has_value()) {
        return false;
    }
    const auto& actions = your_turn->available_actions;
    return std::find(actions.begin(), actions.end(), action) != actions.end();
}

std::string ClientViewState::hand_hint() const
{
    if (!game_state.has_value()) {
        return {};
    }
    return describe_hand(game_state->your_cards, game_state->general_cards);
}

void ClientViewState::apply_message(const poker::protocol::ServerMessage& msg)
{
    std::visit([this](const auto& concrete) {
        using T = std::decay_t<decltype(concrete)>;

        if constexpr (std::is_same_v<T, poker::protocol::Welcome>) {
            player_name = concrete.player_name;
            screen = UiScreen::Lobby;
            append_log(tr(Msg::WelcomeUser, player_name));
            status_message = tr(Msg::WelcomeUser, player_name);
        } else if constexpr (std::is_same_v<T, poker::protocol::RoomList>) {
            rooms = concrete.rooms;
            append_log(tr(Msg::RoomListUpdated, std::to_string(rooms.size())));
        } else if constexpr (std::is_same_v<T, poker::protocol::JoinedRoom>) {
            room_id = concrete.room_id;
            player_names = concrete.player_names;
            room_host_name = concrete.host_name;
            room_max_players = concrete.max_players;
            is_room_host = !player_name.empty() && player_name == concrete.host_name;

            if (concrete.in_game && screen == UiScreen::InGame) {
                append_log(tr(Msg::TableRosterUpdated));
                if (game_state.has_value()) {
                    status_message = tr(Msg::PhasePrefix, tr_phase(game_state->phase));
                    if (!game_state->active_player_name.empty()) {
                        status_message += " — " + tr(Msg::ToActSuffix, game_state->active_player_name);
                    }
                }
            } else {
                game_state.reset();
                your_turn.reset();
                last_hand_result.reset();
                screen = UiScreen::WaitingRoom;
                std::ostringstream names;
                for (size_t i = 0; i < player_names.size(); ++i) {
                    if (i > 0) {
                        names << ", ";
                    }
                    names << player_names[i];
                }
                append_log(tr(Msg::JoinedRoomLog, std::to_string(concrete.room_id), names.str()));
                if (is_room_host) {
                    status_message = tr(Msg::YouAreHost);
                } else {
                    status_message = tr(Msg::WaitingForHostStart, concrete.host_name);
                }
            }
        } else if constexpr (std::is_same_v<T, poker::protocol::GameStateUpdate>) {
            last_hand_result.reset();
            game_state = concrete;
            screen = UiScreen::InGame;
            status_message = tr(Msg::PhasePrefix, tr_phase(concrete.phase));
            if (!concrete.active_player_name.empty()) {
                status_message += " — " + tr(Msg::ToActSuffix, concrete.active_player_name);
            }
        } else if constexpr (std::is_same_v<T, poker::protocol::YourTurn>) {
            your_turn = concrete;
            screen = UiScreen::InGame;
            status_message = tr(Msg::YourTurn);
        } else if constexpr (std::is_same_v<T, poker::protocol::HandResult>) {
            last_hand_result = concrete;
            std::ostringstream winners;
            for (const auto& name : concrete.winner_names) {
                winners << " " << name;
            }
            append_log(tr(Msg::HandWonBy, winners.str(), std::to_string(concrete.pot_amount)));
            status_message = tr(Msg::HandWonBy, winners.str(), std::to_string(concrete.pot_amount));
        } else if constexpr (std::is_same_v<T, poker::protocol::ActionEvent>) {
            append_log(format_action_event(concrete));
        } else if constexpr (std::is_same_v<T, poker::protocol::LeftRoom>) {
            const uint64_t left_id = concrete.room_id;
            reset_play_session();
            screen = UiScreen::Lobby;
            append_log(tr(Msg::LeftRoomLog, std::to_string(left_id)));
            status_message = tr(Msg::InLobby);
        } else if constexpr (std::is_same_v<T, poker::protocol::Error>) {
            append_log(tr(Msg::ErrorPrefix, concrete.description));
            status_message = concrete.description;
        }
    },
        msg);
}

} // namespace poker::client
