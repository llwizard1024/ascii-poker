#include "application.h"

#include "command.h"
#include "network_client.h"

#include <spdlog/spdlog.h>

#include <charconv>
#include <optional>
#include <string_view>

namespace poker::client {

namespace pp = poker::protocol;

namespace {

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

} // namespace

void ClientApplication::process_input(const std::string& line)
{
    const Command cmd = CommandParser::parse(line);

    switch (cmd.type) {
    case Command::Type::CreateRoom:
        if (cmd.args.size() >= 2) {
            const auto max_players = to_uint32(cmd.args[1]);
            if (max_players.has_value() && *max_players >= 2 && *max_players <= 255) {
                create_room(cmd.args[0], static_cast<uint8_t>(*max_players));
            }
        }
        return;
    case Command::Type::JoinRoom:
        if (!cmd.args.empty()) {
            const auto room_id = to_uint64(cmd.args[0]);
            if (room_id.has_value()) {
                join_room(*room_id);
            }
        }
        return;
    case Command::Type::LeaveRoom:
        leave_room();
        return;
    case Command::Type::ListRooms:
        list_rooms();
        return;
    case Command::Type::StartGame:
        start_game();
        return;
    case Command::Type::PlayerAction:
        if (!cmd.args.empty()) {
            player_action_from_command(cmd.args[0], cmd.args);
        }
        return;
    case Command::Type::Quit:
        quit();
        return;
    default:
        if (!cmd.args.empty()) {
            unknown(cmd.args[0]);
        }
        return;
    }
}

void ClientApplication::list_rooms()
{
    client_->send(pp::ClientMessage { pp::ListRooms {} });
    spdlog::info("Sending ListRooms");
}

void ClientApplication::send_hello(const std::string& player_name)
{
    client_->send(pp::ClientMessage { pp::Hello { player_name } });
    spdlog::info("Sending Hello: player_name={}", player_name);
}

void ClientApplication::create_room(const std::string& name, uint8_t max_players)
{
    client_->send(pp::ClientMessage { pp::CreateRoom { name, max_players } });
    spdlog::info("Sending CreateRoom: room_name={}, max_players={}", name, max_players);
}

void ClientApplication::join_room(uint64_t room_id)
{
    client_->send(pp::ClientMessage { pp::JoinRoom { room_id } });
    spdlog::info("Sending JoinRoom: room_id={}", room_id);
}

void ClientApplication::leave_room()
{
    client_->send(pp::ClientMessage { pp::LeaveRoom {} });
    spdlog::info("Sending LeaveRoom");
}

void ClientApplication::start_game()
{
    client_->send(pp::ClientMessage { pp::StartGame {} });
    spdlog::info("Sending StartGame");
}

void ClientApplication::send_player_action(const pp::Action action, const std::optional<uint32_t> amount)
{
    client_->send(pp::ClientMessage { pp::PlayerAction { action, amount } });
    spdlog::info("Sending PlayerAction");
}

void ClientApplication::quit()
{
    spdlog::info("User call Quit.");
    quit_flag_.store(true);
}

bool ClientApplication::is_connected() const
{
    return client_->is_connected();
}

void ClientApplication::reconnect()
{
    client_->reconnect();
}

void ClientApplication::player_action_from_command(const std::string& action_str, const std::vector<std::string>& args)
{
    std::optional<uint32_t> amount = std::nullopt;
    pp::Action action;

    if (action_str == "fold") {
        action = pp::Action::Fold;
    } else if (action_str == "check") {
        action = pp::Action::Check;
    } else if (action_str == "call") {
        action = pp::Action::Call;
    } else if (action_str == "raise") {
        if (args.size() < 2) {
            spdlog::warn("Raise requires amount");
            return;
        }
        amount = to_uint32(args[1]);
        if (!amount.has_value()) {
            spdlog::warn("Invalid amount for raise");
            return;
        }
        action = pp::Action::Raise;
    } else {
        spdlog::warn("Unknown player action: {}", action_str);
        return;
    }

    send_player_action(action, amount);
}

void ClientApplication::unknown(const std::string& token)
{
    spdlog::warn("Unknown command: {}", token);
}

} // namespace poker::client
