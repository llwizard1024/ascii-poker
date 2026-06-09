#pragma once

#include "room.h"
#include "session.h"

#include <memory>
#include <vector>
#include <unordered_map>

class Lobby {
public:
    std::shared_ptr<Room> create_room(const std::string& name, uint8_t max_players, std::shared_ptr<Session> creator);
    bool join_room(uint64_t room_id, std::shared_ptr<Session> player);
    void leave_room(std::shared_ptr<Session> player);
private:
    std::vector<std::shared_ptr<Room>> rooms_;
    std::unordered_map<std::shared_ptr<Session>, std::shared_ptr<Room>> player_rooms_;
    uint64_t next_room_id_ = 1;

    std::shared_ptr<Room> find_room(uint64_t id);
};