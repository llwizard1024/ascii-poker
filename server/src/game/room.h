#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../network/session.h"

class Room {
public:
    Room(uint64_t id, std::string name, uint8_t max_players);
    bool add_player(std::shared_ptr<Session> player);
    bool remove_player(std::shared_ptr<Session> player);
    poker::protocol::RoomInfo get_room_info() const;
    size_t player_count() const { return players_.size(); }
    uint64_t get_id() const { return id_; }
    std::string get_name() const { return name_; }
    std::vector<std::string> get_all_players_names() const;

private:
    uint64_t id_;
    std::string name_;
    uint8_t max_players_;
    std::vector<std::shared_ptr<Session>> players_;

    void notify_all_about_joined();
};