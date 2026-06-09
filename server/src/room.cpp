#include "room.h"

#include <algorithm>

#include <spdlog/spdlog.h>

#include "poker/protocol.h"

Room::Room(uint64_t id, std::string name, uint8_t max_players)
    : id_(id)
    , name_(name)
    , max_players_(max_players)
{
    spdlog::info("Room success created with name - {} and id - {}", name_, id_);
}

bool Room::add_player(std::shared_ptr<Session> player)
{
    if (players_.size() >= max_players_) {
        spdlog::warn("Room {} is full, cannot add player {}", name_, player->get_name());
        return false;
    }

    players_.push_back(player);

    spdlog::info("Player {} added to room {}", player->get_name(), id_);

    notify_all_about_joined();

    if (players_.size() == max_players_) {
        spdlog::info("Room {} is full!", name_);
    }

    return true;
}

bool Room::remove_player(std::shared_ptr<Session> player)
{
    auto it = std::find(players_.begin(), players_.end(), player);

    if (it == players_.end()) {
        spdlog::warn("Player {} not found for remove action!", player->get_name());
        return false;
    }

    players_.erase(it);

    spdlog::info("Player {} removed from room {}", player->get_name(), name_);

    if (!players_.empty()) {
        notify_all_about_joined();
    } else {
        spdlog::info("Room {} is empty, all players quit", name_);
    }

    return true;
}

poker::protocol::RoomInfo Room::get_room_info() const
{
    return poker::protocol::RoomInfo{
        id_,
        name_,
        static_cast<uint8_t>(players_.size()),
        max_players_
    };
}

void Room::notify_all_about_joined()
{
    std::vector<std::string> player_names;

    for (const auto& elem : players_) {
        player_names.push_back(elem->get_name());
    }

    auto joined = poker::protocol::JoinedRoom { id_, std::move(player_names) };

    for (const auto& elem : players_) {
        elem->send_response(poker::protocol::ServerMessage { joined });
    }
}
