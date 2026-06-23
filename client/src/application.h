#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "command.h"

class NetworkClient;

class ClientApplication {
public:
    explicit ClientApplication(std::shared_ptr<NetworkClient> client)
        : client_(client) { };

    void process_input(const std::string& line);

    bool quit_requested() const { return quit_flag_.load(); }

private:
    std::shared_ptr<NetworkClient> client_;
    std::atomic<bool> quit_flag_ { false };

    void create_room(const Command& cmd);
    void join_room(const Command& cmd);
    void leave_room();
    void list_rooms();
    void player_action(const Command& cmd);
    void quit();
    void unknown(const Command& cmd);
};