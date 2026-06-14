#pragma once

#include "../game/lobby.h"
#include "i_message_processor.h"

#include <memory>

class LobbyMessageProcessor : public IMessageProcessor {
public:
    explicit LobbyMessageProcessor(std::shared_ptr<Lobby> lobby);
    ~LobbyMessageProcessor() override;
    void process_message(const std::string& json, std::shared_ptr<Session> session) override;

private:
    std::shared_ptr<Lobby> lobby_;
};