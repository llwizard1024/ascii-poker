#include <iostream>
#include <json/json.hpp>
#include <string>
#include <unordered_set>

#include "spdlog/spdlog.h"
#include "utils/helper.h"

std::unordered_set<std::string> connected_players;

void handle_json(nlohmann::json& data)
{
    std::cout << data["nickname"].is_string() << std::endl;

    if (data.contains("nickname") && data["nickname"].is_string()) {
        if (connected_players.count(data["nickname"]) == 0) {
            connected_players.insert(data["nickname"]);
            return;
        }

        std::cout << data["nickname"] + " player exsists\n";
        return;
    }

    std::cout << "Error handle join\n";
}

int main()
{
    while (true) {
        std::string line;
        std::getline(std::cin, line);

        try {
            nlohmann::json data = nlohmann::json::parse(line);
            if (data.contains("type") && data["type"].is_string() && data["type"] == "join") {
                handle_json(data);
                continue;
            }

            std::cout << "Join not foind" << std::endl;

        } catch (const nlohmann::json::parse_error& e) {
            std::cout << "Parse error: " << e.what() << std::endl;
        }
    }

    return 0;
}