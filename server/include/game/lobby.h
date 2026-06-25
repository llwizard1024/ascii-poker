#pragma once

#include "game/room.h"
#include "network/i_connection.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace poker::server {

class ConnectionRegistry;
class UserRepository;

class Lobby {
public:
    explicit Lobby(
        std::shared_ptr<ConnectionRegistry> connection_registry,
        UserRepository* user_repository = nullptr);

    std::optional<poker::protocol::ErrorCode> check_player_bankroll(ConnectionPtr player) const;
    std::shared_ptr<Room> create_room(const std::string& name, uint8_t max_players, ConnectionPtr creator);
    std::optional<poker::protocol::ErrorCode> join_room(uint64_t room_id, ConnectionPtr player);
    void leave_room(ConnectionPtr player);
    std::optional<uint32_t> account_chips_for(const std::string& username) const;
    std::optional<poker::protocol::ErrorCode> start_game(ConnectionPtr player);
    poker::protocol::RoomList get_room_list() const;
    std::shared_ptr<Room> get_room(uint64_t id) const;
    std::shared_ptr<Room> find_room_by_player(ConnectionPtr player) const;

private:
    void broadcast_room_list();

    std::shared_ptr<ConnectionRegistry> connection_registry_;
    UserRepository* user_repository_ = nullptr;
    std::vector<std::shared_ptr<Room>> rooms_;
    std::unordered_map<ConnectionPtr, std::shared_ptr<Room>> player_rooms_;
    uint64_t next_room_id_ = 1;
};

} // namespace poker::server
