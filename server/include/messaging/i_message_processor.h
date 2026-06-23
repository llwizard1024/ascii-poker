#pragma once

#include "network/i_connection.h"

#include <memory>
#include <string>

namespace poker::server {

class IMessageProcessor {
public:
    virtual ~IMessageProcessor() = default;
    virtual void process_message(const std::string& json, ConnectionPtr connection) = 0;
    virtual void on_disconnect(ConnectionPtr /*connection*/) {}
};

} // namespace poker::server
