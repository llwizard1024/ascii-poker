#include "remote_player.h"

#include "../network/session.h"

RemotePlayer::RemotePlayer(std::shared_ptr<Session> session)
    : session_(std::move(session)) {}

void RemotePlayer::send_message(const poker::protocol::ServerMessage& msg) {
    session_->send_response(msg);
}

std::string RemotePlayer::get_name() const {
    return session_->get_name();
}