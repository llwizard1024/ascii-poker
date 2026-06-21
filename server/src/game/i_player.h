#pragma once

#include <memory>

#include <string>
#include "poker/protocol.h"

class IPlayer {
public:
    virtual ~IPlayer() = default;
    virtual void send_message(const poker::protocol::ServerMessage& msg) = 0;
    virtual std::string get_name() const = 0;
};