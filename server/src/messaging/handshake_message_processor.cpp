#include "messaging/handshake_message_processor.h"

#include "messaging/lobby_message_processor.h"
#include "network/connection_registry.h"
#include "network/i_connection.h"
#include "poker/auth.h"
#include "poker/error_codes.h"
#include "poker/game_constants.h"
#include "poker/protocol.h"
#include "storage/user_repository.h"

#include <json/json.hpp>
#include <spdlog/spdlog.h>

namespace poker::server {

namespace pp = poker::protocol;

HandshakeMessageProcessor::HandshakeMessageProcessor(
    std::shared_ptr<Lobby> lobby,
    std::shared_ptr<PlayerNameRegistry> name_registry,
    std::shared_ptr<ConnectionRegistry> connection_registry,
    UserRepository* user_repository)
    : lobby_(std::move(lobby))
    , name_registry_(std::move(name_registry))
    , connection_registry_(std::move(connection_registry))
    , user_repository_(user_repository)
    , lobby_processor_(std::make_shared<LobbyMessageProcessor>(lobby_))
{
}

HandshakeMessageProcessor::~HandshakeMessageProcessor() = default;

void HandshakeMessageProcessor::finish_authentication(
    const std::string& name,
    ConnectionPtr connection,
    uint32_t chips,
    bool is_new_account)
{
    if (!name_registry_->try_register(name)) {
        connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::AlreadyOnline) });
        return;
    }

    connection->set_player_name(name);
    connection_registry_->add(connection);
    connection->send(pp::ServerMessage {
        pp::Welcome { name, chips, is_new_account } });
    spdlog::info("Player '{}' authenticated (chips={}, new={})", name, chips, is_new_account);
}

void HandshakeMessageProcessor::handle_login(const pp::Login& login, ConnectionPtr connection)
{
    const std::string username = poker::auth::trim_copy(login.username);
    const std::string password = login.password;

    if (!poker::auth::is_valid_username(username)) {
        connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::InvalidPlayerName) });
        return;
    }

    if (!poker::auth::is_valid_password(password)) {
        connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::InvalidPassword) });
        return;
    }

    bool is_new_account = false;
    std::optional<UserRecord> user;

    if (!user_repository_->username_exists(username)) {
        const auto created = user_repository_->create_user(username, password, poker::STARTING_CHIPS);
        if (created == UserCreateResult::Created) {
            is_new_account = true;
            user = user_repository_->find_by_username(username);
        } else if (!user_repository_->verify_password(username, password)) {
            connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::WrongPassword) });
            return;
        } else {
            user = user_repository_->find_by_username(username);
        }
    } else if (!user_repository_->verify_password(username, password)) {
        connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::WrongPassword) });
        return;
    } else {
        user = user_repository_->find_by_username(username);
    }

    if (!user.has_value()) {
        connection->send(pp::ServerMessage {
            pp::make_error(pp::ErrorCode::InvalidMessage, "User record missing after login") });
        return;
    }

    finish_authentication(username, connection, user->chips, is_new_account);
}

void HandshakeMessageProcessor::process_message(const std::string& json, ConnectionPtr connection)
{
    nlohmann::json data;

    try {
        data = nlohmann::json::parse(json);
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error(e.what());
        connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::InvalidJson, e.what()) });
        return;
    }

    const std::string type = data.at("type").get<std::string>();

    if (type == "login") {
        if (connection->is_authenticated()) {
            connection->send(pp::ServerMessage {
                pp::make_error(pp::ErrorCode::InvalidMessage, "Already authenticated") });
            return;
        }

        pp::Login login;
        try {
            login = data.at("data").get<pp::Login>();
        } catch (const std::exception& e) {
            connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::InvalidMessage, e.what()) });
            return;
        }

        handle_login(login, connection);
        return;
    }

    if (!connection->is_authenticated()) {
        connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::NotAuthenticated) });
        return;
    }

    pp::ClientMessage msg;
    try {
        msg = data.get<pp::ClientMessage>();
    } catch (const std::exception& e) {
        connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::InvalidMessage, e.what()) });
        return;
    }

    lobby_processor_->process_client_message(msg, connection);
}

void HandshakeMessageProcessor::on_disconnect(ConnectionPtr connection)
{
    if (connection->is_authenticated()) {
        name_registry_->unregister(connection->get_name());
        connection_registry_->remove(connection);
    }

    lobby_processor_->on_disconnect(connection);
}

} // namespace poker::server
