#pragma once

#include "poker/protocol.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace poker::client {

class NetworkClient;

class ClientApplication {
public:
    explicit ClientApplication(std::shared_ptr<NetworkClient> client)
        : client_(std::move(client)) { };

    void process_input(const std::string& line);

    void list_rooms();
    void send_hello(const std::string& player_name);
    void create_room(const std::string& name, uint8_t max_players);
    void join_room(uint64_t room_id);
    void leave_room();
    void start_game();
    void send_player_action(poker::protocol::Action action, std::optional<uint32_t> amount = std::nullopt);
    void quit();

    bool quit_requested() const { return quit_flag_.load(); }

private:
    std::shared_ptr<NetworkClient> client_;
    std::atomic<bool> quit_flag_ { false };

    void player_action_from_command(const std::string& action_str, const std::vector<std::string>& args);
    void unknown(const std::string& token);
};

} // namespace poker::client
