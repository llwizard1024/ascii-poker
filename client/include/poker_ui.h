#pragma once

#include "application.h"
#include "client_settings.h"
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
    void initialize_session(const ClientSettings& settings, const std::string& host, const std::string& port);
    void on_connection_changed(ConnectionStatus status);

private:
    enum class PendingRefocus {
        None,
        RaiseInput,
        CreateNameInput,
        PlayerNameInput,
        PasswordInput,
    };

    void notify_redraw();
    void rebuild_room_labels();
    void sync_control_visibility();
    void show_create_panel(bool show);
    void submit_create_room();
    void submit_login();
    void submit_raise();
    void send_action(poker::protocol::Action action, std::optional<uint32_t> amount = std::nullopt);
    void send_preset_raise(uint32_t amount);
    void apply_pending_refocus();
    void refresh_button_labels();
    void update_turn_button_labels();
    void toggle_language();
    void lobby_room_step(int delta);
    bool text_input_has_focus() const;

    struct ButtonLabels {
        std::string join;
        std::string quit;
        std::string refresh;
        std::string create;
        std::string leave;
        std::string start;
        std::string reconnect;
        std::string confirm;
        std::string cancel;
        std::string fold;
        std::string check;
        std::string call;
        std::string raise;
        std::string min;
        std::string half;
        std::string pot;
        std::string allin;
        std::string lang;
        std::string room_up;
        std::string room_down;
    } btn_;

    ftxui::Element render_ui() const;
    ftxui::Element render_header() const;
    ftxui::Element render_body() const;
    ftxui::Element render_login() const;
    ftxui::Element render_lobby() const;
    ftxui::Element render_waiting_room() const;
    ftxui::Element render_game_table() const;
    ftxui::Element render_log_panel() const;
    ftxui::Element render_status_bar() const;
    ftxui::Element render_disconnected() const;
    ftxui::Element render_hand_result_banner() const;
    bool handle_hotkeys(ftxui::Event event);

    std::shared_ptr<ClientApplication> app_;
    ftxui::ScreenInteractive* screen_ = nullptr;

    ftxui::Component raise_input_;
    ftxui::Component create_name_input_;
    ftxui::Component create_size_input_;
    ftxui::Component player_name_input_;
    ftxui::Component login_password_input_;
    ftxui::Component login_host_input_;
    ftxui::Component login_port_input_;

    mutable std::mutex mtx_;
    ClientViewState state_;

    int selected_room_index_ = 0;
    int log_scroll_offset_ = 0;
    std::vector<std::string> room_labels_;
    bool show_create_panel_ = false;
    std::string create_room_name_;
    std::string create_max_players_ = "4";
    std::string player_name_input_value_;
    std::string login_password_value_;
    std::string login_host_value_;
    std::string login_port_value_;
    bool show_login_controls_ = true;
    bool show_lobby_controls_ = false;
    bool show_create_controls_ = false;
    bool show_waiting_controls_ = false;
    bool show_waiting_start_ = false;
    bool show_game_controls_ = false;
    bool show_turn_actions_ = false;
    bool show_fold_ = false;
    bool show_check_ = false;
    bool show_call_ = false;
    bool show_raise_ = false;
    bool show_raise_presets_ = false;
    bool show_disconnected_controls_ = false;
    std::string raise_amount_;
    bool action_pending_ = false;
    std::optional<poker::protocol::YourTurn> saved_your_turn_;
    PendingRefocus pending_refocus_ = PendingRefocus::None;
    bool login_password_mode_ = true;
    bool pending_auto_login_ = false;
    bool ring_bell_ = false;
};

} // namespace poker::client
