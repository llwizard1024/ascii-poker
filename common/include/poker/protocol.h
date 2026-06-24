#pragma once

#include "poker/card.h"
#include "poker/error_codes.h"

#include <cstdint>
#include <json/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace poker {
inline void to_json(nlohmann::json& j, const Card& card)
{
    j = card.to_string();
}

inline void from_json(const nlohmann::json& j, Card& card)
{
    std::string str = j.get<std::string>();

    if (str.size() < 2) {
        throw std::runtime_error("Invalid card string length: " + str);
    }

    char suit_char = str.back();
    Suit suit;
    switch (suit_char) {
    case 'C':
        suit = Suit::Clubs;
        break;
    case 'D':
        suit = Suit::Diamonds;
        break;
    case 'H':
        suit = Suit::Hearts;
        break;
    case 'S':
        suit = Suit::Spades;
        break;
    default:
        throw std::runtime_error(std::string("Unknown card suit: ") + suit_char);
    }

    std::string rank_str = str.substr(0, str.size() - 1);
    Rank rank;
    if (rank_str == "10") {
        rank = Rank::Ten;
    } else if (rank_str.size() == 1) {
        switch (rank_str[0]) {
        case '2':
            rank = Rank::Two;
            break;
        case '3':
            rank = Rank::Three;
            break;
        case '4':
            rank = Rank::Four;
            break;
        case '5':
            rank = Rank::Five;
            break;
        case '6':
            rank = Rank::Six;
            break;
        case '7':
            rank = Rank::Seven;
            break;
        case '8':
            rank = Rank::Eight;
            break;
        case '9':
            rank = Rank::Nine;
            break;
        case 'J':
            rank = Rank::Jack;
            break;
        case 'Q':
            rank = Rank::Queen;
            break;
        case 'K':
            rank = Rank::King;
            break;
        case 'A':
            rank = Rank::Ace;
            break;
        default:
            throw std::invalid_argument("Unknown card rank: " + rank_str);
        }
    } else {
        throw std::invalid_argument("Unknown card rank: " + rank_str);
    }

    card = Card(suit, rank);
}

inline void from_json(const nlohmann::json& j, std::vector<Card>& cards)
{
    cards.clear();
    for (const auto& item : j) {
        Card card(Suit::Clubs, Rank::Two);
        from_json(item, card);
        cards.push_back(card);
    }
}
}

namespace poker::protocol {
enum class Action {
    Fold,
    Check,
    Call,
    Raise
};
NLOHMANN_JSON_SERIALIZE_ENUM(Action, { { Action::Fold, "fold" }, { Action::Check, "check" }, { Action::Call, "call" }, { Action::Raise, "raise" } })

enum class GamePhase {
    PreFlop,
    Flop,
    Turn,
    River,
    Showdown
};
NLOHMANN_JSON_SERIALIZE_ENUM(GamePhase, { { GamePhase::PreFlop, "preflop" }, { GamePhase::Flop, "flop" }, { GamePhase::Turn, "turn" }, { GamePhase::River, "river" }, { GamePhase::Showdown, "showdown" } })

// Client Messages
struct Hello {
    std::string player_name;
};
void to_json(nlohmann::json& j, const Hello& msg);
void from_json(const nlohmann::json& j, Hello& msg);

struct CreateRoom {
    std::string room_name;
    uint8_t max_players;
};
void to_json(nlohmann::json& j, const CreateRoom& msg);
void from_json(const nlohmann::json& j, CreateRoom& msg);

struct JoinRoom {
    uint64_t room_id;
};
void to_json(nlohmann::json& j, const JoinRoom& msg);
void from_json(const nlohmann::json& j, JoinRoom& msg);

struct LeaveRoom { };
void to_json(nlohmann::json& j, const LeaveRoom& msg);
void from_json(const nlohmann::json& j, LeaveRoom& msg);

struct ListRooms { };
void to_json(nlohmann::json& j, const ListRooms& msg);
void from_json(const nlohmann::json& j, ListRooms& msg);

struct StartGame { };
void to_json(nlohmann::json& j, const StartGame& msg);
void from_json(const nlohmann::json& j, StartGame& msg);

struct PlayerAction {
    Action action;
    std::optional<uint32_t> amount;
};
void to_json(nlohmann::json& j, const PlayerAction& msg);
void from_json(const nlohmann::json& j, PlayerAction& msg);

// Server Messages
struct Welcome {
    std::string player_name;
};
void to_json(nlohmann::json& j, const Welcome& msg);
void from_json(const nlohmann::json& j, Welcome& msg);

struct RoomInfo {
    uint64_t room_id;
    std::string room_name;
    uint8_t current_players;
    uint8_t max_players;
    bool in_game = false;
};
void to_json(nlohmann::json& j, const RoomInfo& msg);
void from_json(const nlohmann::json& j, RoomInfo& msg);

struct RoomList {
    std::vector<RoomInfo> rooms;
};
void to_json(nlohmann::json& j, const RoomList& msg);
void from_json(const nlohmann::json& j, RoomList& msg);

struct JoinedRoom {
    uint64_t room_id;
    std::vector<std::string> player_names;
    std::string host_name;
    uint8_t max_players;
    bool in_game = false;
};
void to_json(nlohmann::json& j, const JoinedRoom& msg);
void from_json(const nlohmann::json& j, JoinedRoom& msg);

struct PlayerState {
    std::string name;
    uint32_t chips = 0;
    uint32_t round_bet = 0;
    bool folded = false;
    bool all_in = false;
    std::vector<Card> hole_cards;
};
void to_json(nlohmann::json& j, const PlayerState& msg);
void from_json(const nlohmann::json& j, PlayerState& msg);

struct GameStateUpdate {
    std::vector<Card> general_cards;
    std::vector<Card> your_cards;
    GamePhase phase;
    std::string active_player_name;
    uint32_t total_pot = 0;
    uint32_t current_bet = 0;
    std::vector<PlayerState> players;
};
void to_json(nlohmann::json& j, const GameStateUpdate& msg);
void from_json(const nlohmann::json& j, GameStateUpdate& msg);

struct YourTurn {
    std::vector<Action> available_actions;
    uint32_t min_amount = 0;
    uint32_t max_amount = 0;
    uint32_t to_call = 0;
    uint32_t your_chips = 0;
};
void to_json(nlohmann::json& j, const YourTurn& msg);
void from_json(const nlohmann::json& j, YourTurn& msg);

struct ActionEvent {
    std::string player_name;
    Action action;
    std::optional<uint32_t> amount;
};
void to_json(nlohmann::json& j, const ActionEvent& msg);
void from_json(const nlohmann::json& j, ActionEvent& msg);

struct Error {
    int code;
    std::string description;
};
Error make_error(ErrorCode code, std::string detail = {});
void to_json(nlohmann::json& j, const Error& msg);
void from_json(const nlohmann::json& j, Error& msg);

struct LeftRoom {
    uint64_t room_id;
};
void to_json(nlohmann::json& j, const LeftRoom& msg);
void from_json(const nlohmann::json& j, LeftRoom& msg);

struct HandResult {
    std::vector<std::string> winner_names;
    uint32_t pot_amount;
};
void to_json(nlohmann::json& j, const HandResult& msg);
void from_json(const nlohmann::json& j, HandResult& msg);

using ClientMessage = std::variant<Hello, CreateRoom, JoinRoom, LeaveRoom, ListRooms, StartGame, PlayerAction>;
void to_json(nlohmann::json& j, const ClientMessage& msg);
void from_json(const nlohmann::json& j, ClientMessage& msg);

using ServerMessage = std::variant<Welcome, RoomList, JoinedRoom, GameStateUpdate, YourTurn, Error, LeftRoom, HandResult, ActionEvent>;
void to_json(nlohmann::json& j, const ServerMessage& msg);
void from_json(const nlohmann::json& j, ServerMessage& msg);
}