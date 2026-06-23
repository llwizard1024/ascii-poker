#pragma once

#include "poker/protocol.h"

#include <string>

namespace poker::server {

class IPlayer {
public:
    virtual ~IPlayer() = default;
    virtual void send_message(const poker::protocol::ServerMessage& msg) = 0;
    virtual std::string get_name() const = 0;
};

} // namespace poker::server
