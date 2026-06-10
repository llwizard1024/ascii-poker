#pragma once

#include "room.h"
#include "session.h"

#include <memory>
#include <unordered_map>
#include <vector>

class Lobby {
public:
    std::shared_ptr<Room> create_room(const std::string& name, uint8_t max_players, std::shared_ptr<Session> creator);
    bool join_room(uint64_t room_id, std::shared_ptr<Session> player);
    void leave_room(std::shared_ptr<Session> player);
    poker::protocol::RoomList get_room_list() const;

private:
    std::vector<std::shared_ptr<Room>> rooms_;
    std::unordered_map<std::shared_ptr<Session>, std::shared_ptr<Room>> player_rooms_;
    uint64_t next_room_id_ = 1;
};
