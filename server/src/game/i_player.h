#pragma once

#include <memory>

#include "poker/protocol.h"

#include <string>

class IPlayer {
public:
    virtual ~IPlayer() = default;
    virtual void send_message(const poker::protocol::ServerMessage& msg) = 0;
    virtual std::string get_name() const = 0;
};