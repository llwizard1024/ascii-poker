#pragma once

#include "poker/protocol.h"

#include <memory>
#include <string>

namespace poker::server {

class IConnection {
public:
    virtual ~IConnection() = default;
    virtual void send(const poker::protocol::ServerMessage& msg) = 0;
    virtual std::string get_name() const = 0;
    virtual bool is_connected() const { return true; }
    virtual bool is_authenticated() const { return true; }
    virtual void set_player_name(std::string name) = 0;
};

using ConnectionPtr = std::shared_ptr<IConnection>;

} // namespace poker::server
