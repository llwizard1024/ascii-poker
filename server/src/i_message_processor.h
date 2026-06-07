#pragma once

#include <memory>
#include <string>

class Session;

class IMessageProcessor {
public:
    virtual ~IMessageProcessor() = default;
    virtual void process_message(const std::string& json, std::shared_ptr<Session> session) = 0;
};