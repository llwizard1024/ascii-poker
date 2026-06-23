#pragma once

#include "game/lobby.h"
#include "messaging/i_message_processor.h"

#include <poker/protocol.h>

#include <memory>

namespace poker::server {

class LobbyMessageProcessor : public IMessageProcessor {
public:
    explicit LobbyMessageProcessor(std::shared_ptr<Lobby> lobby);
    ~LobbyMessageProcessor() override;
    void process_message(const std::string& json, ConnectionPtr connection) override;
    void process_client_message(const poker::protocol::ClientMessage& msg, ConnectionPtr connection);
    void on_disconnect(ConnectionPtr connection) override;

private:
    std::shared_ptr<Lobby> lobby_;
};

} // namespace poker::server
