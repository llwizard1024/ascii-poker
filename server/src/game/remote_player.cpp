#include "game/remote_player.h"

namespace poker::server {

RemotePlayer::RemotePlayer(ConnectionPtr connection)
    : connection_(std::move(connection))
{
}

void RemotePlayer::send_message(const poker::protocol::ServerMessage& msg)
{
    if (connection_ && connection_->is_connected()) {
        connection_->send(msg);
    }
}

std::string RemotePlayer::get_name() const
{
    return connection_->get_name();
}

} // namespace poker::server
