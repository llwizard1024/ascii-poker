#include "messaging/handshake_message_processor.h"

#include "messaging/lobby_message_processor.h"
#include "network/connection_registry.h"
#include "network/i_connection.h"
#include "poker/error_codes.h"
#include "poker/protocol.h"

#include <json/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>

namespace poker::server {


namespace pp = poker::protocol;

namespace {

constexpr size_t kMinPlayerNameLength = 1;
constexpr size_t kMaxPlayerNameLength = 32;


std::string trim_copy(std::string value)
{
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}


bool is_valid_player_name(const std::string& name)
{
    if (name.size() < kMinPlayerNameLength || name.size() > kMaxPlayerNameLength) {
        return false;
    }

    for (unsigned char ch : name) {
        if (std::iscntrl(ch)) {
            return false;
        }
    }

    return true;
}

} // namespace


HandshakeMessageProcessor::HandshakeMessageProcessor(
    std::shared_ptr<Lobby> lobby,
    std::shared_ptr<PlayerNameRegistry> name_registry,
    std::shared_ptr<ConnectionRegistry> connection_registry)
    : lobby_(std::move(lobby))
    , name_registry_(std::move(name_registry))
    , connection_registry_(std::move(connection_registry))
    , lobby_processor_(std::make_shared<LobbyMessageProcessor>(lobby_))
{
}


HandshakeMessageProcessor::~HandshakeMessageProcessor() = default;


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

    if (type == "hello") {
        if (connection->is_authenticated()) {
            connection->send(pp::ServerMessage {
                pp::make_error(pp::ErrorCode::InvalidMessage, "Already authenticated") });
            return;
        }

        pp::Hello hello;
        try {
            hello = data.at("data").get<pp::Hello>();
        } catch (const std::exception& e) {
            connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::InvalidMessage, e.what()) });
            return;
        }

        const std::string name = trim_copy(std::move(hello.player_name));
        if (!is_valid_player_name(name)) {
            connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::InvalidPlayerName) });
            return;
        }

        if (!name_registry_->try_register(name)) {
            connection->send(pp::ServerMessage { pp::make_error(pp::ErrorCode::NameTaken) });
            return;
        }

        connection->set_player_name(name);
        connection_registry_->add(connection);
        connection->send(pp::ServerMessage { pp::Welcome { name } });
        spdlog::info("Player '{}' authenticated", name);
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
