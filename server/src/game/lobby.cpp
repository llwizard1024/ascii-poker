#include "game/lobby.h"

#include "network/connection_registry.h"
#include "poker/error_codes.h"
#include "poker/game_constants.h"
#include "storage/user_repository.h"

#include <spdlog/spdlog.h>

#include <algorithm>

namespace poker::server {

Lobby::Lobby(std::shared_ptr<ConnectionRegistry> connection_registry, UserRepository* user_repository)
    : connection_registry_(std::move(connection_registry))
    , user_repository_(user_repository)
{
}

std::optional<poker::protocol::ErrorCode> Lobby::check_player_bankroll(ConnectionPtr player) const
{
    if (!user_repository_) {
        return std::nullopt;
    }

    const auto user = user_repository_->find_by_username(player->get_name());
    const uint32_t chips = user.has_value() ? user->chips : 0;
    if (chips < poker::BIG_BLIND) {
        return poker::protocol::ErrorCode::NotEnoughChips;
    }

    return std::nullopt;
}

void Lobby::broadcast_room_list()
{
    if (!connection_registry_) {
        return;
    }

    connection_registry_->broadcast(poker::protocol::ServerMessage { get_room_list() });
}

std::shared_ptr<Room> Lobby::create_room(const std::string& name, uint8_t max_players, ConnectionPtr creator)
{
    if (max_players < 2) {
        spdlog::warn("Cannot create room: max_players must be at least 2");
        return nullptr;
    }

    if (const auto error = check_player_bankroll(creator)) {
        spdlog::warn("Cannot create room: creator '{}' has insufficient chips", creator->get_name());
        return nullptr;
    }

    const auto player_rooms_it = player_rooms_.find(creator);

    if (player_rooms_it != player_rooms_.end()) {
        leave_room(creator);
    }

    std::shared_ptr<Room> room = std::make_shared<Room>(
        next_room_id_++, name, max_players, creator, user_repository_);
    rooms_.push_back(room);

    room->add_player(creator);

    player_rooms_.insert({ creator, room });

    spdlog::info("Room {} created", room->get_name());

    broadcast_room_list();

    return room;
}

std::optional<poker::protocol::ErrorCode> Lobby::join_room(uint64_t room_id, ConnectionPtr player)
{
    if (const auto bankroll_error = check_player_bankroll(player)) {
        return bankroll_error;
    }

    if (player_rooms_.count(player) > 0 && player_rooms_[player]->get_id() == room_id) {
        spdlog::info("You are already in this room");
        return std::nullopt;
    }

    const auto player_rooms_it = player_rooms_.find(player);

    if (player_rooms_it != player_rooms_.end()) {
        leave_room(player);
    }

    const auto room_it = std::find_if(rooms_.begin(), rooms_.end(),
        [&room_id](const std::shared_ptr<Room>& room) {
            return room->get_id() == room_id;
        });

    if (room_it == rooms_.end()) {
        spdlog::warn("Error join to room {}", room_id);
        return poker::protocol::ErrorCode::JoinFailed;
    }

    const auto& room = *room_it;
    if (room->is_game_started()) {
        spdlog::warn("Room {} game already started", room_id);
        return poker::protocol::ErrorCode::GameAlreadyStarted;
    }

    if (room->player_count() >= room->get_room_info().max_players) {
        spdlog::warn("Room {} is full", room_id);
        return poker::protocol::ErrorCode::RoomFull;
    }

    const bool player_added = room->add_player(player);

    if (player_added) {
        player_rooms_.insert({ player, room });
        spdlog::info("Player {} entered the room {}", player->get_name(), room->get_name());
        broadcast_room_list();
        return std::nullopt;
    }

    spdlog::warn("Failed to enter the room");
    return poker::protocol::ErrorCode::JoinFailed;
}

void Lobby::leave_room(ConnectionPtr player)
{
    const auto player_rooms_it = player_rooms_.find(player);

    if (player_rooms_it == player_rooms_.end()) {
        return;
    }

    const auto room = player_rooms_it->second;
    room->remove_player(player);
    player_rooms_.erase(player_rooms_it);

    if (room->player_count() == 0) {
        rooms_.erase(
            std::remove(rooms_.begin(), rooms_.end(), room),
            rooms_.end());
    }

    broadcast_room_list();
}

std::optional<uint32_t> Lobby::account_chips_for(const std::string& username) const
{
    if (!user_repository_) {
        return std::nullopt;
    }

    const auto user = user_repository_->find_by_username(username);
    if (!user.has_value()) {
        return std::nullopt;
    }

    return user->chips;
}

std::optional<poker::protocol::ErrorCode> Lobby::start_game(ConnectionPtr player)
{
    const auto room = find_room_by_player(player);
    if (!room) {
        return poker::protocol::ErrorCode::NotInRoom;
    }

    if (const auto error = room->try_start_game(player)) {
        return error;
    }

    broadcast_room_list();
    return std::nullopt;
}

poker::protocol::RoomList Lobby::get_room_list() const
{
    poker::protocol::RoomList room_list;

    for (const auto& elem : rooms_) {
        room_list.rooms.push_back(elem->get_room_info());
    }

    return room_list;
}

std::shared_ptr<Room> Lobby::get_room(uint64_t id) const
{
    const auto it = std::find_if(rooms_.begin(), rooms_.end(), [id](const auto& room) {
        return room->get_id() == id;
    });

    return it == rooms_.end() ? nullptr : *it;
}

std::shared_ptr<Room> Lobby::find_room_by_player(ConnectionPtr player) const
{
    const auto it = player_rooms_.find(player);
    return it != player_rooms_.end() ? it->second : nullptr;
}

} // namespace poker::server
