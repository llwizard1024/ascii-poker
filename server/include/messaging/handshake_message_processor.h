#pragma once

#include "messaging/i_message_processor.h"
#include "messaging/lobby_message_processor.h"
#include "network/connection_registry.h"
#include "network/player_name_registry.h"

#include <memory>

namespace poker::server {

class UserRepository;

class HandshakeMessageProcessor : public IMessageProcessor {
public:
    HandshakeMessageProcessor(
        std::shared_ptr<Lobby> lobby,
        std::shared_ptr<PlayerNameRegistry> name_registry,
        std::shared_ptr<ConnectionRegistry> connection_registry,
        UserRepository* user_repository);
    ~HandshakeMessageProcessor() override;

    void process_message(const std::string& json, ConnectionPtr connection) override;
    void on_disconnect(ConnectionPtr connection) override;

private:
    void handle_login(const poker::protocol::Login& login, ConnectionPtr connection);
    void finish_authentication(const std::string& name, ConnectionPtr connection, uint32_t chips, bool is_new_account);

    std::shared_ptr<Lobby> lobby_;
    std::shared_ptr<PlayerNameRegistry> name_registry_;
    std::shared_ptr<ConnectionRegistry> connection_registry_;
    UserRepository* user_repository_ = nullptr;
    std::shared_ptr<LobbyMessageProcessor> lobby_processor_;
};

} // namespace poker::server
