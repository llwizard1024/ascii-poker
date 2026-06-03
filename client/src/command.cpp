#include "command.h"

Command CommandParser::parse(const std::string& input)
{
    std::vector<std::string> tokens = split_input(input);
    if (tokens.empty())
        return { Command::Type::Unknown, {} };

    Command::Type type = type_from_string(tokens[0]);

    std::vector<std::string> args(
        std::make_move_iterator(tokens.begin() + 1),
        std::make_move_iterator(tokens.end()));

    return { type, std::move(args) };
}

std::vector<std::string> CommandParser::split_input(const std::string& input)
{
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;

    for (char c : input) {
        if (c == '"') {
            in_quotes = !in_quotes;
            current += c;
        } else if (c == ' ' && !in_quotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

Command::Type CommandParser::type_from_string(const std::string& token)
{
    std::string clean = token;
    if (clean.size() >= 2 && clean.front() == '"' && clean.back() == '"') {
        clean = clean.substr(1, clean.size() - 2);
    }

    if (clean == "create_room")
        return Command::Type::CreateRoom;
    if (clean == "join_room")
        return Command::Type::JoinRoom;
    if (clean == "leave_room")
        return Command::Type::LeaveRoom;
    if (clean == "action")
        return Command::Type::PlayerAction;
    if (clean == "quit")
        return Command::Type::Quit;
    return Command::Type::Unknown;
}