#pragma once

#include "network/i_connection.h"
#include "poker/protocol.h"

#include <memory>
#include <unordered_set>

namespace poker::server {

class ConnectionRegistry {
public:
    void add(ConnectionPtr connection);
    void remove(ConnectionPtr connection);
    void broadcast(const poker::protocol::ServerMessage& message);

private:
    std::unordered_set<ConnectionPtr> connections_;
};

} // namespace poker::server
