#pragma once

#include <string_view>

namespace poker::protocol {

enum class ErrorCode : int {
    InvalidJson = 1,
    InvalidMessage = 2,
    JoinFailed = 3,
    NotInRoom = 4,
    NotYourTurn = 5,
    CannotCheck = 6,
    InvalidRaise = 7,
    NotEnoughChips = 8,
    InvalidMaxPlayers = 9,
    NameTaken = 10,
    InvalidPlayerName = 11,
    NotAuthenticated = 12,
    RoomFull = 13,
    NotRoomHost = 14,
    NotEnoughPlayers = 15,
    GameAlreadyStarted = 16,
};

constexpr std::string_view error_code_message(ErrorCode code)
{
    switch (code) {
    case ErrorCode::InvalidJson:
        return "Invalid JSON";
    case ErrorCode::InvalidMessage:
        return "Invalid message";
    case ErrorCode::JoinFailed:
        return "Join to room failed";
    case ErrorCode::NotInRoom:
        return "Not in a room";
    case ErrorCode::NotYourTurn:
        return "Not your turn";
    case ErrorCode::CannotCheck:
        return "Cannot check – there is a bet";
    case ErrorCode::InvalidRaise:
        return "Invalid raise amount";
    case ErrorCode::NotEnoughChips:
        return "Not enough chips";
    case ErrorCode::InvalidMaxPlayers:
        return "max_players must be at least 2";
    case ErrorCode::NameTaken:
        return "Player name is already taken";
    case ErrorCode::InvalidPlayerName:
        return "Invalid player name";
    case ErrorCode::NotAuthenticated:
        return "Send hello with a player name first";
    case ErrorCode::RoomFull:
        return "Room is full";
    case ErrorCode::NotRoomHost:
        return "Only the room host can start the game";
    case ErrorCode::NotEnoughPlayers:
        return "At least 2 players are required to start";
    case ErrorCode::GameAlreadyStarted:
        return "Game has already started";
    }
    return "Unknown error";
}

} // namespace poker::protocol
