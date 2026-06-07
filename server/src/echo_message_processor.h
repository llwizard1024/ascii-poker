#pragma once

#include "i_message_processor.h"

class EchoMessageProcessor : public IMessageProcessor {
public:
    ~EchoMessageProcessor() override;
    void process_message(const std::string& json, std::shared_ptr<Session> session) override;
};