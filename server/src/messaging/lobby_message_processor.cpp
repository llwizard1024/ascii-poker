#include "lobby_message_processor.h"

#include "../network/session.h"
#include "poker/protocol.h"

#include <spdlog/spdlog.h>

namespace pp = poker::protocol;

LobbyMessageProcessor::~LobbyMessageProcessor() = default;

LobbyMessageProcessor::LobbyMessageProcessor(std::shared_ptr<Lobby> lobby)
    : lobby_(std::move(lobby))
{
}

void LobbyMessageProcessor::process_message(const std::string& json, std::shared_ptr<Session> session)
{
    nlohmann::json data;

    try {
        data = nlohmann::json::parse(json);
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error(e.what());

        session->send_response(pp::ServerMessage { pp::Error { 1, "Invalid JSON: " + std::string(e.what()) } });
        return;
    }

    pp::ClientMessage msg;

    try {
        msg = data.get<pp::ClientMessage>();
    } catch (const std::exception& e) {
        spdlog::error(e.what());

        session->send_response(pp::ServerMessage { pp::Error { 2, e.what() } });
        return;
    }

    std::visit([this, session](const auto& concrete_msg) {
        using T = std::decay_t<decltype(concrete_msg)>;
        if constexpr (std::is_same_v<T, pp::CreateRoom>) {
            spdlog::info("Create Room Message (Lobby Processor)");

            if (concrete_msg.max_players < 2) {
                session->send_response(pp::ServerMessage {
                    pp::Error { 9, "max_players must be at least 2" } });
                return;
            }

            lobby_->create_room(concrete_msg.room_name, concrete_msg.max_players, session);
        } else if constexpr (std::is_same_v<T, pp::JoinRoom>) {
            spdlog::info("Join Room Message (Lobby Processor)");

            bool join_success = lobby_->join_room(concrete_msg.room_id, session);
            if (!join_success) {
                session->send_response(pp::ServerMessage { pp::Error { 3, "Join to room failed" } });
            }
        } else if constexpr (std::is_same_v<T, pp::LeaveRoom>) {
            spdlog::info("Leave Room Message (Lobby Processor)");

            auto room = lobby_->find_room_by_player(session);
            const uint64_t room_id = room ? room->get_id() : 0;
            lobby_->leave_room(session);
            session->send_response(pp::ServerMessage { pp::LeftRoom { room_id } });
        } else if constexpr (std::is_same_v<T, pp::ListRooms>) {
            spdlog::info("List Rooms Message (Lobby Processor)");

            session->send_response(pp::ServerMessage { lobby_->get_room_list() });
        } else if constexpr (std::is_same_v<T, pp::PlayerAction>) {
            spdlog::info("Player Action Message (Lobby Processor)");

            auto room = lobby_->find_room_by_player(session);
            if (room) {
                room->on_player_action(session, concrete_msg.action, concrete_msg.amount);
            } else {
                session->send_response(pp::ServerMessage { pp::Error { 4, "Not in a room" } });
            }
        }
    },
        msg);
}

void LobbyMessageProcessor::on_disconnect(std::shared_ptr<Session> session)
{
    spdlog::info("Client disconnected: {}", session->get_name());
    lobby_->leave_room(session);
}
