#pragma once

#include "network/i_connection.h"
#include "poker/protocol.h"

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace poker::test {

class MockConnection : public poker::server::IConnection {
public:
    explicit MockConnection(std::string name)
        : name_(std::move(name))
    {
    }

    void send(const poker::protocol::ServerMessage& msg) override
    {
        sent_messages_.push_back(msg);
    }

    std::string get_name() const override { return name_; }
    bool is_authenticated() const override { return authenticated_; }
    void set_player_name(std::string name) override
    {
        name_ = std::move(name);
        authenticated_ = true;
    }

    const std::vector<poker::protocol::ServerMessage>& sent_messages() const { return sent_messages_; }

    void clear_messages() { sent_messages_.clear(); }

private:
    std::string name_;
    bool authenticated_ = true;
    std::vector<poker::protocol::ServerMessage> sent_messages_;
};

template <typename T>
bool has_message(const MockConnection& connection)
{
    for (const auto& message : connection.sent_messages()) {
        if (std::holds_alternative<T>(message)) {
            return true;
        }
    }
    return false;
}

template <typename T>
std::optional<T> find_message(const MockConnection& connection)
{
    for (const auto& message : connection.sent_messages()) {
        if (std::holds_alternative<T>(message)) {
            return std::get<T>(message);
        }
    }
    return std::nullopt;
}

} // namespace poker::test
