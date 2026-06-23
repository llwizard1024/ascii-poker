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
};

struct ClientViewState {
    UiScreen screen = UiScreen::Login;

    std::string player_name;

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
};

std::string phase_to_string(poker::protocol::GamePhase phase);
std::string action_to_string(poker::protocol::Action action);

} // namespace poker::client
