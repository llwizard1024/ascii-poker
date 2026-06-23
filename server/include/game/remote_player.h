#pragma once

#include "game/i_player.h"
#include "network/i_connection.h"

#include <memory>

namespace poker::server {

class RemotePlayer : public IPlayer {
public:
    explicit RemotePlayer(ConnectionPtr connection);
    void send_message(const poker::protocol::ServerMessage& msg) override;
    std::string get_name() const override;

private:
    ConnectionPtr connection_;
};

} // namespace poker::server
