#include "game/room.h"

#include "game/remote_player.h"
#include "poker/error_codes.h"
#include "poker/game_constants.h"
#include "poker/protocol.h"
#include "storage/user_repository.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <unordered_map>

namespace poker::server {

Room::Room(
    uint64_t id,
    std::string name,
    uint8_t max_players,
    ConnectionPtr host,
    UserRepository* user_repository)
    : user_repository_(user_repository)
    , id_(id)
    , name_(std::move(name))
    , max_players_(max_players)
    , host_(std::move(host))
{
    spdlog::info("Room success created with name - {} and id - {}", name_, id_);
}

std::optional<poker::protocol::ErrorCode> Room::load_player_bankrolls(
    std::unordered_map<std::string, uint32_t>& bankrolls) const
{
    for (const auto& connection : players_) {
        uint32_t chips = poker::STARTING_CHIPS;

        if (user_repository_) {
            const auto user = user_repository_->find_by_username(connection->get_name());
            chips = user.has_value() ? user->chips : 0;
        }

        if (chips < poker::BIG_BLIND) {
            spdlog::warn(
                "Player '{}' has insufficient chips ({}) to play in room {}",
                connection->get_name(),
                chips,
                id_);
            return poker::protocol::ErrorCode::NotEnoughChips;
        }

        bankrolls[connection->get_name()] = chips;
    }

    return std::nullopt;
}

bool Room::add_player(ConnectionPtr player)
{
    if (game_session_) {
        spdlog::warn("Room {} game already started, cannot add player {}", name_, player->get_name());
        return false;
    }

    if (players_.size() >= max_players_) {
        spdlog::warn("Room {} is full, cannot add player {}", name_, player->get_name());
        return false;
    }

    players_.push_back(player);

    spdlog::info("Player {} added to room {}", player->get_name(), id_);

    notify_all_about_joined();

    if (players_.size() == max_players_) {
        spdlog::info("Room {} is full, auto-starting game", name_);
        if (const auto error = start_game()) {
            spdlog::warn("Room {} auto-start failed", name_);
            (void)error;
        }
    }

    return true;
}

bool Room::remove_player(ConnectionPtr player)
{
    const auto it = std::find(players_.begin(), players_.end(), player);

    if (it == players_.end()) {
        spdlog::warn("Player {} not found for remove action!", player->get_name());
        return false;
    }

    if (game_session_) {
        game_session_->remove_player(player);
        if (!game_session_->has_enough_players()) {
            game_session_.reset();
        }
    }

    players_.erase(it);

    if (player == host_ && !players_.empty()) {
        host_ = players_.front();
        spdlog::info("Room {} host transferred to {}", name_, host_->get_name());
    }

    spdlog::info("Player {} removed from room {}", player->get_name(), id_);

    if (!players_.empty()) {
        notify_all_about_joined();
    } else {
        spdlog::info("Room {} is empty, all players quit", name_);
    }

    return true;
}

poker::protocol::RoomInfo Room::get_room_info() const
{
    return poker::protocol::RoomInfo {
        id_,
        name_,
        static_cast<uint8_t>(players_.size()),
        max_players_,
        game_session_ != nullptr
    };
}

poker::protocol::JoinedRoom Room::make_joined_room_message() const
{
    return poker::protocol::JoinedRoom {
        id_,
        get_all_players_names(),
        host_->get_name(),
        max_players_,
        game_session_ != nullptr
    };
}

void Room::notify_all_about_joined()
{
    const auto joined = make_joined_room_message();

    for (const auto& connection : players_) {
        connection->send(poker::protocol::ServerMessage { joined });
    }
}

std::vector<std::string> Room::get_all_players_names() const
{
    std::vector<std::string> player_names;

    for (const auto& connection : players_) {
        player_names.push_back(connection->get_name());
    }

    return player_names;
}

std::optional<poker::protocol::ErrorCode> Room::try_start_game(ConnectionPtr requester)
{
    if (game_session_) {
        return poker::protocol::ErrorCode::GameAlreadyStarted;
    }

    if (requester != host_) {
        return poker::protocol::ErrorCode::NotRoomHost;
    }

    if (players_.size() < 2) {
        return poker::protocol::ErrorCode::NotEnoughPlayers;
    }

    return start_game();
}

std::optional<poker::protocol::ErrorCode> Room::start_game()
{
    if (game_session_) {
        return std::nullopt;
    }

    std::unordered_map<std::string, uint32_t> bankrolls;
    if (const auto error = load_player_bankrolls(bankrolls)) {
        return error;
    }

    std::vector<std::shared_ptr<IPlayer>> game_players;
    for (const auto& connection : players_) {
        game_players.push_back(std::make_shared<RemotePlayer>(connection));
    }

    std::unordered_map<IConnection*, IPlayer*> connection_map;
    for (size_t i = 0; i < players_.size(); ++i) {
        connection_map[players_[i].get()] = game_players[i].get();
    }

    game_session_ = std::make_unique<GameSession>(
        game_players,
        connection_map,
        id_,
        user_repository_,
        bankrolls);
    game_session_->start();
    return std::nullopt;
}

void Room::on_player_action(ConnectionPtr player, poker::protocol::Action action, std::optional<uint32_t> amount)
{
    if (game_session_) {
        game_session_->apply_action(player, action, amount);
    }
}

} // namespace poker::server
