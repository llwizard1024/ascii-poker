#pragma once

#include "ftxui/ftxui_all.hpp"
#include "application.h"
#include "poker/protocol.h"

#include <memory>
#include <deque>
#include <string>
#include <mutex>

class PokerUI {
public:
    PokerUI(std::shared_ptr<ClientApplication> app);
    void set_screen(ftxui::ScreenInteractive& screen);
    void add_message(const std::string& msg);
    void run();
    void add_server_message(const poker::protocol::ServerMessage& msg);

private:
    std::shared_ptr<ClientApplication> app_;
    ftxui::ScreenInteractive* screen_ = nullptr;
    std::deque<std::string> messages_;
    std::mutex mtx_;
};