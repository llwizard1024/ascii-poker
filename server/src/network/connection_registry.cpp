#include "network/connection_registry.h"

namespace poker::server {

void ConnectionRegistry::add(ConnectionPtr connection)
{
    connections_.insert(std::move(connection));
}

void ConnectionRegistry::remove(ConnectionPtr connection)
{
    connections_.erase(connection);
}

void ConnectionRegistry::broadcast(const poker::protocol::ServerMessage& message)
{
    for (const auto& connection : connections_) {
        connection->send(message);
    }
}

} // namespace poker::server
