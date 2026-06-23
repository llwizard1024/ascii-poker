#pragma once

#include <string>
#include <vector>

namespace poker::client {

struct Command {
    // clang-format off
    enum class Type { CreateRoom, JoinRoom, LeaveRoom, ListRooms, StartGame, PlayerAction, Quit, Unknown };
    // clang-format on
    Type type;
    std::vector<std::string> args;
};

class CommandParser {
public:
    static Command parse(const std::string& input);

private:
    static std::vector<std::string> split_input(const std::string& input);
    static Command::Type type_from_string(const std::string& token);
};

} // namespace poker::client
