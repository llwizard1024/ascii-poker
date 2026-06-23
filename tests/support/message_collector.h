#pragma once

#include "network_client.h"
#include "poker/protocol.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <vector>

namespace poker::test {

class MessageCollector {
public:
    void attach(const std::shared_ptr<poker::client::NetworkClient>& client)
    {
        client->set_message_handler([this](const poker::protocol::ServerMessage& msg) {
            std::lock_guard lock(mutex_);
            messages_.push_back(msg);
            cv_.notify_all();
        });
    }

    template <typename T>
    bool wait_for(std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        std::unique_lock lock(mutex_);
        while (std::chrono::steady_clock::now() < deadline) {
            for (const auto& msg : messages_) {
                if (std::holds_alternative<T>(msg)) {
                    return true;
                }
            }
            cv_.wait_for(lock, std::chrono::milliseconds(10));
        }
        return false;
    }

    template <typename T>
    std::optional<T> find() const
    {
        std::lock_guard lock(mutex_);
        for (const auto& msg : messages_) {
            if (std::holds_alternative<T>(msg)) {
                return std::get<T>(msg);
            }
        }
        return std::nullopt;
    }

    void clear()
    {
        std::lock_guard lock(mutex_);
        messages_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<poker::protocol::ServerMessage> messages_;
};

} // namespace poker::test
