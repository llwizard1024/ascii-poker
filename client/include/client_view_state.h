#pragma once

#include "poker/protocol.h"

#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace poker::client {

enum class UiScreen {
    Login,
    Lobby,
    WaitingRoom,
    InGame,
    Disconnected,
};

enum class ConnectionStatus {
    Connecting,
    Connected,
    Failed,
    Disconnected
};

struct ClientViewState {
    UiScreen screen = UiScreen::Login;
    ConnectionStatus connection = ConnectionStatus::Connecting;

    std::string player_name;
    std::string server_host;
    std::string server_port;

    std::vector<poker::protocol::RoomInfo> rooms;
    std::optional<uint64_t> room_id;
    std::vector<std::string> player_names;
    std::string room_host_name;
    uint8_t room_max_players = 0;
    bool is_room_host = false;

    std::optional<poker::protocol::GameStateUpdate> game_state;
    std::optional<poker::protocol::YourTurn> your_turn;
    std::optional<poker::protocol::HandResult> last_hand_result;

    std::deque<std::string> log;
    std::string status_message;

    void append_log(std::string message);
    void apply_message(const poker::protocol::ServerMessage& msg);
    void set_connection(ConnectionStatus status);
    void reset_play_session();

    bool has_action(poker::protocol::Action action) const;
    std::string hand_hint() const;
};

std::string phase_to_string(poker::protocol::GamePhase phase);
std::string action_to_string(poker::protocol::Action action);
std::string connection_status_label(ConnectionStatus status);

} // namespace poker::client
