#include "lobby.h"

#include <spdlog/spdlog.h>

#include <algorithm>

std::shared_ptr<Room> Lobby::create_room(const std::string& name, uint8_t max_players, std::shared_ptr<Session> creator)
{
    auto player_rooms_it = player_rooms_.find(creator);

    if (player_rooms_it != player_rooms_.end()) {
        leave_room(creator);
    }

    std::shared_ptr<Room> room = std::make_shared<Room>(next_room_id_++, name, max_players);
    rooms_.push_back(room);

    room->add_player(creator);

    player_rooms_.insert({ creator, room });

    spdlog::info("Room {} created", room->get_name());

    return room;
}

bool Lobby::join_room(uint64_t room_id, std::shared_ptr<Session> player)
{
    if (player_rooms_.count(player) > 0 && player_rooms_[player]->get_id() == room_id) {
        spdlog::info("You are already in this room");
        return true;
    }

    auto player_rooms_it = player_rooms_.find(player);

    if (player_rooms_it != player_rooms_.end()) {
        leave_room(player);
    }

    auto room_it = std::find_if(rooms_.begin(), rooms_.end(),
        [&room_id](const std::shared_ptr<Room>& room) {
            return room->get_id() == room_id;
        });

    if (room_it == rooms_.end()) {
        spdlog::warn("Error join to room {}", room_id);
        return false;
    }

    bool player_added = room_it->get()->add_player(player);

    if (player_added) {
        player_rooms_.insert({ player, *room_it });
        spdlog::info("Player {} entered the room {}", player->get_name(), room_it->get()->get_name());
        return true;
    }

    spdlog::warn("Failed to enter the room");
    return false;
}

void Lobby::leave_room(std::shared_ptr<Session> player)
{
    auto player_rooms_it = player_rooms_.find(player);

    if (player_rooms_it == player_rooms_.end()) {
        return;
    }

    auto room = player_rooms_it->second;
    room->remove_player(player);
    player_rooms_.erase(player_rooms_it);

    if (room->player_count() == 0) {
        rooms_.erase(
            std::remove(rooms_.begin(), rooms_.end(), room),
            rooms_.end());
    }
}

poker::protocol::RoomList Lobby::get_room_list() const
{
    poker::protocol::RoomList room_list;

    for (const auto& elem : rooms_) {
        room_list.rooms.push_back(elem->get_room_info());
    }

    return room_list;
}
