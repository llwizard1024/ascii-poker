#include "echo_message_processor.h"

#include "poker/protocol.h"
#include "session.h"

#include <spdlog/spdlog.h>

namespace pp = poker::protocol;

EchoMessageProcessor::~EchoMessageProcessor() = default;

void EchoMessageProcessor::process_message(const std::string& json, std::shared_ptr<Session> session)
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

    std::visit([](const auto& concrete_msg) {
        using T = std::decay_t<decltype(concrete_msg)>;
        if constexpr (std::is_same_v<T, pp::CreateRoom>) {
            spdlog::info("Create Room Message");
        } else if constexpr (std::is_same_v<T, pp::JoinRoom>) {
            spdlog::info("Join Room Message");
        } else if constexpr (std::is_same_v<T, pp::LeaveRoom>) {
            spdlog::info("Leave Room Message");
        } else if constexpr (std::is_same_v<T, pp::PlayerAction>) {
            spdlog::info("Player Action Message");
        }
    },
        msg);

    session->send_response(pp::ServerMessage { pp::Error { 0, "echo: " + json } });
}