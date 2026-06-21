#pragma once

#include "i_player.h"

#include <memory>

class Session;

class RemotePlayer : public IPlayer {
public:
    explicit RemotePlayer(std::shared_ptr<Session> session);
    void send_message(const poker::protocol::ServerMessage& msg) override;
    std::string get_name() const override;
private:
    std::shared_ptr<Session> session_;
};