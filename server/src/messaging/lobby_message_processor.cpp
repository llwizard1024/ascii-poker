#include "messaging/lobby_message_processor.h"

#include "network/session.h"
#include "poker/error_codes.h"
#include "poker/protocol.h"

#include <json/json.hpp>
#include <spdlog/spdlog.h>

namespace poker::server {

namespace pp = poker::protocol;

LobbyMessageProcessor::~LobbyMessageProcessor() = default;

LobbyMessageProcessor::LobbyMessageProcessor(std::shared_ptr<Lobby> lobby)
    : lobby_(std::move(lobby))
{
}

void LobbyMessageProcessor::process_message(const std::string& json, ConnectionPtr connection)
{
    nlohmann::json data;

    try {
        data = nlohmann::json::parse(json);
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error(e.what());
        connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::InvalidJson, e.what()) });
        return;
    }

    pp::ClientMessage msg;

    try {
        msg = data.get<pp::ClientMessage>();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::InvalidMessage, e.what()) });
        return;
    }

    process_client_message(msg, connection);
}

void LobbyMessageProcessor::process_client_message(const pp::ClientMessage& msg, ConnectionPtr connection)
{
    std::visit([this, connection](const auto& concrete_msg) {
        using T = std::decay_t<decltype(concrete_msg)>;
        if constexpr (std::is_same_v<T, pp::CreateRoom>) {
            spdlog::info("Create Room Message (Lobby Processor)");

            if (concrete_msg.max_players < 2) {
                connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::InvalidMaxPlayers) });
                return;
            }

            if (const auto error = lobby_->check_player_bankroll(connection)) {
                connection->send(pp::ServerMessage { pp::make_error(*error) });
                return;
            }

            lobby_->create_room(concrete_msg.room_name, concrete_msg.max_players, connection);
        } else if constexpr (std::is_same_v<T, pp::JoinRoom>) {
            spdlog::info("Join Room Message (Lobby Processor)");

            if (const auto error = lobby_->join_room(concrete_msg.room_id, connection)) {
                connection->send(pp::ServerMessage { pp::make_error(*error) });
            }
        } else if constexpr (std::is_same_v<T, pp::LeaveRoom>) {
            spdlog::info("Leave Room Message (Lobby Processor)");

            const auto room = lobby_->find_room_by_player(connection);
            const uint64_t room_id = room ? room->get_id() : 0;
            lobby_->leave_room(connection);

            pp::LeftRoom left { room_id };
            if (const auto chips = lobby_->account_chips_for(connection->get_name())) {
                left.your_chips = *chips;
                left.has_your_chips = true;
            }
            connection->send(pp::ServerMessage { left });
        } else if constexpr (std::is_same_v<T, pp::ListRooms>) {
            spdlog::info("List Rooms Message (Lobby Processor)");
            connection->send(pp::ServerMessage { lobby_->get_room_list() });
        } else if constexpr (std::is_same_v<T, pp::StartGame>) {
            spdlog::info("Start Game Message (Lobby Processor)");

            if (const auto error = lobby_->start_game(connection)) {
                connection->send(pp::ServerMessage { pp::make_error(*error) });
            }
        } else if constexpr (std::is_same_v<T, pp::PlayerAction>) {
            spdlog::info("Player Action Message (Lobby Processor)");

            const auto room = lobby_->find_room_by_player(connection);
            if (room) {
                room->on_player_action(connection, concrete_msg.action, concrete_msg.amount);
            } else {
                connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::NotInRoom) });
            }
        }
    },
        msg);
}

void LobbyMessageProcessor::on_disconnect(ConnectionPtr connection)
{
    spdlog::info("Client disconnected: {}", connection->get_name());
    lobby_->leave_room(connection);
}

} // namespace poker::server
