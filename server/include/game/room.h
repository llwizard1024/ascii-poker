#pragma once

#include "game/game_session.h"
#include "network/i_connection.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace poker::server {

class UserRepository;

class Room {
public:
    Room(
        uint64_t id,
        std::string name,
        uint8_t max_players,
        ConnectionPtr host,
        UserRepository* user_repository = nullptr);
    bool add_player(ConnectionPtr player);
    bool remove_player(ConnectionPtr player);
    poker::protocol::RoomInfo get_room_info() const;
    poker::protocol::JoinedRoom make_joined_room_message() const;
    size_t player_count() const { return players_.size(); }
    uint64_t get_id() const { return id_; }
    std::string get_name() const { return name_; }
    std::vector<std::string> get_all_players_names() const;
    bool is_game_started() const { return game_session_ != nullptr; }
    std::optional<poker::protocol::ErrorCode> try_start_game(ConnectionPtr requester);
    void on_player_action(ConnectionPtr player, poker::protocol::Action action, std::optional<uint32_t> amount);

private:
    std::optional<poker::protocol::ErrorCode> start_game();
    std::optional<poker::protocol::ErrorCode> load_player_bankrolls(
        std::unordered_map<std::string, uint32_t>& bankrolls) const;
    void notify_all_about_joined();

    UserRepository* user_repository_ = nullptr;
    uint64_t id_;
    std::string name_;
    uint8_t max_players_;
    ConnectionPtr host_;
    std::vector<ConnectionPtr> players_;
    std::unique_ptr<GameSession> game_session_;
};

} // namespace poker::server
