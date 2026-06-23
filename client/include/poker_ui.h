#pragma once

#include "application.h"
#include "client_view_state.h"
#include "ftxui_all.hpp"
#include "poker/protocol.h"

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace poker::client {

class PokerUI {
public:
    PokerUI(std::shared_ptr<ClientApplication> app);
    void run();
    void add_server_message(const poker::protocol::ServerMessage& msg);

private:
    enum class PendingRefocus {
        None,
        RaiseInput,
        CreateNameInput,
        PlayerNameInput,
    };

    void notify_redraw();
    void rebuild_room_labels();
    void sync_control_visibility();
    void show_create_panel(bool show);
    void submit_create_room();
    void submit_hello();
    void submit_raise();
    void send_action(poker::protocol::Action action, std::optional<uint32_t> amount = std::nullopt);
    void apply_pending_refocus();

    ftxui::Element render_ui() const;
    ftxui::Element render_header() const;
    ftxui::Element render_body() const;
    ftxui::Element render_login() const;
    ftxui::Element render_lobby() const;
    ftxui::Element render_waiting_room() const;
    ftxui::Element render_game_table() const;
    ftxui::Element render_log_panel() const;
    ftxui::Element render_status_bar() const;
    bool handle_lobby_navigation(ftxui::Event event);

    std::shared_ptr<ClientApplication> app_;
    ftxui::ScreenInteractive* screen_ = nullptr;

    ftxui::Component raise_input_;
    ftxui::Component create_name_input_;
    ftxui::Component player_name_input_;

    mutable std::mutex mtx_;
    ClientViewState state_;

    int selected_room_index_ = 0;
    std::vector<std::string> room_labels_;
    bool show_create_panel_ = false;
    std::string create_room_name_;
    std::string create_max_players_ = "4";
    std::string player_name_input_value_;
    bool show_login_controls_ = true;
    bool show_lobby_controls_ = false;
    bool show_create_controls_ = false;
    bool show_waiting_controls_ = false;
    bool show_waiting_start_ = false;
    bool show_game_controls_ = false;
    bool show_turn_actions_ = false;
    std::string raise_amount_;
    bool action_pending_ = false;
    std::optional<poker::protocol::YourTurn> saved_your_turn_;
    PendingRefocus pending_refocus_ = PendingRefocus::None;
};

} // namespace poker::client
