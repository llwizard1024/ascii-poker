#include "application.h"

#include "network_client.h"
#include "poker/protocol.h"

#include <spdlog/spdlog.h>

#include <charconv>
#include <optional>
#include <string_view>

namespace pp = poker::protocol;

std::optional<uint32_t> to_uint32(std::string_view str)
{
    uint32_t value = 0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    if (ec == std::errc {} && ptr == str.data() + str.size()) {
        return value;
    }
    return std::nullopt;
}

std::optional<uint64_t> to_uint64(std::string_view str)
{
    uint64_t value = 0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    if (ec == std::errc {} && ptr == str.data() + str.size()) {
        return value;
    }
    return std::nullopt;
}

void ClientApplication::process_input(const std::string& line)
{
    Command cmd = CommandParser::parse(line);

    switch (cmd.type) {
    case Command::Type::CreateRoom:
        create_room(cmd);
        return;
    case Command::Type::JoinRoom:
        join_room(cmd);
        return;
    case Command::Type::LeaveRoom:
        leave_room();
        return;
    case Command::Type::PlayerAction:
        player_action(cmd);
        return;
    case Command::Type::Quit:
        quit();
        return;
    default:
        unknown(cmd);
        return;
    }
}

void ClientApplication::create_room(const Command& cmd)
{
    if (cmd.args.size() < 2) {
        spdlog::warn("Not enough arguments for command: create room");
        return;
    }

    const std::string& arg = cmd.args[1];

    auto parsed_value = to_uint32(arg);

    if (!parsed_value.has_value() || parsed_value.value() > 255) {
        spdlog::error("Failed to parse max_players: invalid argument or value out of range (0-255). Got: '{}'", arg);
        return;
    }

    uint8_t max_players = static_cast<uint8_t>(parsed_value.value());

    client_->send(pp::ClientMessage { pp::CreateRoom { cmd.args[0], max_players } });

    spdlog::info("Sending CreateRoom: room_name={}, max_players={}", cmd.args[0], max_players);
}

void ClientApplication::join_room(const Command& cmd)
{
    if (cmd.args.size() < 1) {
        spdlog::warn("Not enough arguments for command: join room");
        return;
    }

    const std::string& arg = cmd.args[0];
    auto room_id = to_uint64(arg);

    if (!room_id.has_value()) {
        spdlog::error("Failed to parse room_id: invalid integer value. Got: '{}'", arg);
        return;
    }

    client_->send(pp::ClientMessage { pp::JoinRoom { room_id.value() } });

    spdlog::info("Sending JoinRoom: room_id={}", room_id.value());
}

void ClientApplication::leave_room()
{
    client_->send(pp::ClientMessage { pp::LeaveRoom {} });

    spdlog::info("Sending LeaveRoom");
}

void ClientApplication::player_action(const Command& cmd)
{
    if (cmd.args.empty()) {
        spdlog::warn("PlayerAction requires action type");
        return;
    }
    const auto& action_str = cmd.args[0];
    std::optional<uint32_t> amount = std::nullopt;

    pp::Action action;
    if (action_str == "fold") {
        action = pp::Action::Fold;
    } else if (action_str == "check") {
        action = pp::Action::Check;
    } else if (action_str == "call") {
        action = pp::Action::Call;
    } else if (action_str == "raise") {
        if (cmd.args.size() < 2) {
            spdlog::warn("Raise requires amount");
            return;
        }
        amount = to_uint32(cmd.args[1]);
        if (!amount.has_value()) {
            spdlog::warn("Invalid amount for raise");
            return;
        }
        action = pp::Action::Raise;
    } else {
        spdlog::warn("Unknown player action: {}", action_str);
        return;
    }

    client_->send(pp::ClientMessage { pp::PlayerAction { action, amount } });
    spdlog::info("Sending PlayerAction: action={}, amount={}",
        action_str, amount.has_value() ? std::to_string(*amount) : "none");
}

void ClientApplication::quit()
{
    spdlog::info("User call Quit.");

    quit_flag_.store(true);
}

void ClientApplication::unknown(const Command& cmd)
{
    if (cmd.args.size() < 1) {
        spdlog::warn("Not enough arguments for command: unknown");
        return;
    }

    spdlog::warn("Unknown command: {}", cmd.args[0]);
}
