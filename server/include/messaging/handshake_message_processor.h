#pragma once

#include "messaging/i_message_processor.h"
#include "messaging/lobby_message_processor.h"
#include "network/connection_registry.h"
#include "network/player_name_registry.h"

#include <memory>

namespace poker::server {

class HandshakeMessageProcessor : public IMessageProcessor {
public:
    HandshakeMessageProcessor(
        std::shared_ptr<Lobby> lobby,
        std::shared_ptr<PlayerNameRegistry> name_registry,
        std::shared_ptr<ConnectionRegistry> connection_registry);
    ~HandshakeMessageProcessor() override;

    void process_message(const std::string& json, ConnectionPtr connection) override;
    void on_disconnect(ConnectionPtr connection) override;

private:
    std::shared_ptr<Lobby> lobby_;
    std::shared_ptr<PlayerNameRegistry> name_registry_;
    std::shared_ptr<ConnectionRegistry> connection_registry_;
    std::shared_ptr<LobbyMessageProcessor> lobby_processor_;
};

} // namespace poker::server
