#include "poker/protocol.h"

namespace poker::protocol {

Error make_error(ErrorCode code, std::string detail)
{
    std::string description = std::string(error_code_message(code));
    if (!detail.empty()) {
        description += ": ";
        description += detail;
    }
    return Error { static_cast<int>(code), std::move(description) };
}

void to_json(nlohmann::json& j, const Hello& msg)
{
    j = nlohmann::json { { "player_name", msg.player_name } };
}

void from_json(const nlohmann::json& j, Hello& msg)
{
    j.at("player_name").get_to(msg.player_name);
}

void to_json(nlohmann::json& j, const CreateRoom& msg)
{
    j = nlohmann::json { { "room_name", msg.room_name }, { "max_players", msg.max_players } };
}

void from_json(const nlohmann::json& j, CreateRoom& msg)
{
    j.at("room_name").get_to(msg.room_name);
    j.at("max_players").get_to(msg.max_players);
}

void to_json(nlohmann::json& j, const JoinRoom& msg)
{
    j = nlohmann::json { { "room_id", msg.room_id } };
}

void from_json(const nlohmann::json& j, JoinRoom& msg)
{
    j.at("room_id").get_to(msg.room_id);
}

void to_json(nlohmann::json& j, const LeaveRoom&)
{
    j = nlohmann::json::object();
}

void from_json(const nlohmann::json&, LeaveRoom&)
{
    // empty
}

void to_json(nlohmann::json& j, const ListRooms&)
{
    j = nlohmann::json::object();
}

void from_json(const nlohmann::json&, ListRooms&)
{
    // empty
}

void to_json(nlohmann::json& j, const StartGame&)
{
    j = nlohmann::json::object();
}

void from_json(const nlohmann::json&, StartGame&)
{
    // empty
}

void to_json(nlohmann::json& j, const PlayerAction& msg)
{
    j = nlohmann::json { { "action", msg.action } };
    if (msg.amount.has_value()) {
        j["amount"] = msg.amount.value();
    } else {
        j["amount"] = nullptr;
    }
}

void from_json(const nlohmann::json& j, PlayerAction& msg)
{
    j.at("action").get_to(msg.action);

    if (j.contains("amount") && !j["amount"].is_null()) {
        msg.amount = j.at("amount").get<uint32_t>();
    } else {
        msg.amount = std::nullopt;
    }
}

// Server messages

void to_json(nlohmann::json& j, const Welcome& msg)
{
    j = nlohmann::json { { "player_name", msg.player_name } };
}

void from_json(const nlohmann::json& j, Welcome& msg)
{
    j.at("player_name").get_to(msg.player_name);
}

void to_json(nlohmann::json& j, const RoomInfo& msg)
{
    j = nlohmann::json {
        { "room_id", msg.room_id },
        { "room_name", msg.room_name },
        { "current_players", msg.current_players },
        { "max_players", msg.max_players },
        { "in_game", msg.in_game }
    };
}

void from_json(const nlohmann::json& j, RoomInfo& msg)
{
    j.at("room_id").get_to(msg.room_id);
    j.at("room_name").get_to(msg.room_name);
    j.at("current_players").get_to(msg.current_players);
    j.at("max_players").get_to(msg.max_players);
    if (j.contains("in_game")) {
        j.at("in_game").get_to(msg.in_game);
    } else {
        msg.in_game = false;
    }
}

void to_json(nlohmann::json& j, const RoomList& msg)
{
    j = nlohmann::json { { "rooms", msg.rooms } };
}

void from_json(const nlohmann::json& j, RoomList& msg)
{
    j.at("rooms").get_to(msg.rooms);
}

void to_json(nlohmann::json& j, const JoinedRoom& msg)
{
    j = nlohmann::json {
        { "room_id", msg.room_id },
        { "player_names", msg.player_names },
        { "host_name", msg.host_name },
        { "max_players", msg.max_players }
    };
}

void from_json(const nlohmann::json& j, JoinedRoom& msg)
{
    j.at("room_id").get_to(msg.room_id);
    j.at("player_names").get_to(msg.player_names);
    j.at("host_name").get_to(msg.host_name);
    j.at("max_players").get_to(msg.max_players);
}

void to_json(nlohmann::json& j, const GameStateUpdate& msg)
{
    j = nlohmann::json {
        { "general_cards", msg.general_cards },
        { "your_cards", msg.your_cards },
        { "phase", msg.phase },
        { "active_player_name", msg.active_player_name },
        { "total_pot", msg.total_pot }
    };
}

void from_json(const nlohmann::json& j, GameStateUpdate& msg)
{
    j.at("general_cards").get_to(msg.general_cards);
    j.at("your_cards").get_to(msg.your_cards);

    j.at("phase").get_to(msg.phase);
    j.at("active_player_name").get_to(msg.active_player_name);
    j.at("total_pot").get_to(msg.total_pot);
}

void to_json(nlohmann::json& j, const YourTurn& msg)
{
    j = nlohmann::json {
        { "available_actions", msg.available_actions },
        { "min_amount", msg.min_amount },
        { "max_amount", msg.max_amount }
    };
}

void from_json(const nlohmann::json& j, YourTurn& msg)
{
    j.at("available_actions").get_to(msg.available_actions);
    j.at("min_amount").get_to(msg.min_amount);
    j.at("max_amount").get_to(msg.max_amount);
}

void to_json(nlohmann::json& j, const Error& msg)
{
    j = nlohmann::json { { "code", msg.code }, { "description", msg.description } };
}

void from_json(const nlohmann::json& j, Error& msg)
{
    j.at("code").get_to(msg.code);
    j.at("description").get_to(msg.description);
}

void to_json(nlohmann::json& j, const LeftRoom& msg)
{
    j = nlohmann::json { { "room_id", msg.room_id } };
}

void from_json(const nlohmann::json& j, LeftRoom& msg)
{
    j.at("room_id").get_to(msg.room_id);
}

void to_json(nlohmann::json& j, const HandResult& msg)
{
    j = nlohmann::json {
        { "winner_names", msg.winner_names },
        { "pot_amount", msg.pot_amount }
    };
}

void from_json(const nlohmann::json& j, HandResult& msg)
{
    j.at("winner_names").get_to(msg.winner_names);
    j.at("pot_amount").get_to(msg.pot_amount);
}

void to_json(nlohmann::json& j, const ClientMessage& msg)
{
    std::visit([&j](const auto& concrete) {
        using T = std::decay_t<decltype(concrete)>;
        if constexpr (std::is_same_v<T, Hello>) {
            j["type"] = "hello";
        } else if constexpr (std::is_same_v<T, CreateRoom>) {
            j["type"] = "create_room";
        } else if constexpr (std::is_same_v<T, JoinRoom>) {
            j["type"] = "join_room";
        } else if constexpr (std::is_same_v<T, LeaveRoom>) {
            j["type"] = "leave_room";
        } else if constexpr (std::is_same_v<T, ListRooms>) {
            j["type"] = "list_rooms";
        } else if constexpr (std::is_same_v<T, StartGame>) {
            j["type"] = "start_game";
        } else if constexpr (std::is_same_v<T, PlayerAction>) {
            j["type"] = "player_action";
        }
        j["data"] = concrete;
    },
        msg);
}

void from_json(const nlohmann::json& j, ClientMessage& msg)
{
    const auto type = j.at("type").get<std::string>();
    if (type == "hello") {
        msg = j.at("data").get<Hello>();
    } else if (type == "create_room") {
        msg = j.at("data").get<CreateRoom>();
    } else if (type == "join_room") {
        msg = j.at("data").get<JoinRoom>();
    } else if (type == "leave_room") {
        msg = j.at("data").get<LeaveRoom>();
    } else if (type == "list_rooms") {
        msg = j.at("data").get<ListRooms>();
    } else if (type == "start_game") {
        msg = j.at("data").get<StartGame>();
    } else if (type == "player_action") {
        msg = j.at("data").get<PlayerAction>();
    } else {
        throw std::invalid_argument("Unknown ClientMessage type: " + type);
    }
}

void to_json(nlohmann::json& j, const ServerMessage& msg)
{
    std::visit([&j](const auto& concrete) {
        using T = std::decay_t<decltype(concrete)>;
        if constexpr (std::is_same_v<T, Welcome>) {
            j["type"] = "welcome";
        } else if constexpr (std::is_same_v<T, RoomList>) {
            j["type"] = "room_list";
        } else if constexpr (std::is_same_v<T, JoinedRoom>) {
            j["type"] = "joined_room";
        } else if constexpr (std::is_same_v<T, GameStateUpdate>) {
            j["type"] = "game_state_update";
        } else if constexpr (std::is_same_v<T, YourTurn>) {
            j["type"] = "your_turn";
        } else if constexpr (std::is_same_v<T, Error>) {
            j["type"] = "error";
        } else if constexpr (std::is_same_v<T, LeftRoom>) {
            j["type"] = "left_room";
        } else if constexpr (std::is_same_v<T, HandResult>) {
            j["type"] = "hand_result";
        }
        j["data"] = concrete;
    },
        msg);
}

void from_json(const nlohmann::json& j, ServerMessage& msg)
{
    const auto type = j.at("type").get<std::string>();
    if (type == "welcome") {
        msg = j.at("data").get<Welcome>();
    } else if (type == "room_list") {
        msg = j.at("data").get<RoomList>();
    } else if (type == "joined_room") {
        msg = j.at("data").get<JoinedRoom>();
    } else if (type == "game_state_update") {
        msg = j.at("data").get<GameStateUpdate>();
    } else if (type == "your_turn") {
        msg = j.at("data").get<YourTurn>();
    } else if (type == "error") {
        msg = j.at("data").get<Error>();
    } else if (type == "left_room") {
        msg = j.at("data").get<LeftRoom>();
    } else if (type == "hand_result") {
        msg = j.at("data").get<HandResult>();
    } else {
        throw std::invalid_argument("Unknown ServerMessage type: " + type);
    }
}
}