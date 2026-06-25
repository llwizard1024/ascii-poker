#include "poker_ui.h"

#include "client_settings.h"
#include "game_text.h"
#include "i18n.h"
#include "poker/auth.h"
#include "poker/game_constants.h"
#include "ui_cards.h"
#include "ui_theme.h"

#include <charconv>
#include <ftxui_all.hpp>

#include <algorithm>
#include <iostream>
#include <optional>
#include <sstream>
#include <variant>

namespace poker::client {

using namespace ftxui;

namespace {

    std::string hand_result_text(
        const poker::protocol::HandResult& result,
        const std::optional<poker::protocol::GameStateUpdate>& game_state)
    {
        std::ostringstream ss;
        ss << tr(Msg::WinnerPrefix);
        for (size_t i = 0; i < result.winner_names.size(); ++i) {
            ss << " " << result.winner_names[i];
            if (game_state.has_value()) {
                const std::string label = winner_hand_label(result.winner_names[i], *game_state);
                if (!label.empty()) {
                    ss << " (" << label << ")";
                }
            }
        }
        ss << "  |  " << tr(Msg::PotAmountPrefix) << " " << result.pot_amount;
        return ss.str();
    }

    std::string room_label(const poker::protocol::RoomInfo& room)
    {
        std::string label = tr(Msg::RoomLabel,
            std::to_string(room.room_id),
            room.room_name,
            std::to_string(static_cast<int>(room.current_players)),
            std::to_string(static_cast<int>(room.max_players)));
        if (room.in_game) {
            label += tr(Msg::RoomInGame);
        }
        return label;
    }

    std::string trim_label(std::string value)
    {
        const auto start = value.find_first_not_of(' ');
        if (start == std::string::npos) {
            return {};
        }
        const auto end = value.find_last_not_of(' ');
        return value.substr(start, end - start + 1);
    }

} // namespace

PokerUI::PokerUI(std::shared_ptr<ClientApplication> app)
    : app_(std::move(app))
{
}

void PokerUI::initialize_session(const ClientSettings& settings, const std::string& host, const std::string& port)
{
    std::lock_guard lock(mtx_);
    state_.server_host = host;
    state_.server_port = port;
    login_host_value_ = host;
    login_port_value_ = port;
    state_.set_connection(ConnectionStatus::Connecting);
    if (!settings.player_name.empty()) {
        player_name_input_value_ = settings.player_name;
    }
    set_language(parse_language(settings.language));
    sync_control_visibility();
}

void PokerUI::refresh_button_labels()
{
    btn_.join = tr(Msg::BtnJoin);
    btn_.quit = tr(Msg::BtnQuit);
    btn_.refresh = tr(Msg::BtnRefresh);
    btn_.create = tr(Msg::BtnCreate);
    btn_.leave = tr(Msg::BtnLeave);
    btn_.start = tr(Msg::BtnStart);
    btn_.reconnect = tr(Msg::BtnReconnect);
    btn_.confirm = tr(Msg::BtnConfirm);
    btn_.cancel = tr(Msg::BtnCancel);
    btn_.fold = tr(Msg::BtnFold);
    btn_.check = tr(Msg::BtnCheck);
    btn_.call = tr(Msg::BtnCall);
    btn_.raise = tr(Msg::BtnRaise);
    btn_.min = tr(Msg::BtnMin);
    btn_.half = tr(Msg::BtnHalfPot);
    btn_.pot = tr(Msg::BtnPot);
    btn_.allin = tr(Msg::BtnAllIn);
    btn_.lang = tr(Msg::BtnLang);
    btn_.room_up = tr(Msg::BtnRoomUp);
    btn_.room_down = tr(Msg::BtnRoomDown);
}

void PokerUI::update_turn_button_labels()
{
    btn_.call = tr(Msg::BtnCall);
    if (state_.your_turn.has_value() && state_.your_turn->to_call > 0) {
        btn_.call = tr(Msg::BtnCallAmount, std::to_string(state_.your_turn->to_call));
    }
}

void PokerUI::toggle_language()
{
    set_language(current_language() == Language::English ? Language::Russian : Language::English);

    ClientSettings settings;
    {
        std::lock_guard lock(mtx_);
        refresh_button_labels();
        update_turn_button_labels();
        rebuild_room_labels();
        settings.player_name = state_.player_name;
        settings.host = state_.server_host;
        settings.port = state_.server_port;
    }
    settings.language = language_code(current_language());
    save_client_settings(settings);
    notify_redraw();
}

void PokerUI::on_connection_changed(const ConnectionStatus status)
{
    bool auto_login = false;
    {
        std::lock_guard lock(mtx_);
        state_.set_connection(status);
        sync_control_visibility();
        if (pending_auto_login_ && status == ConnectionStatus::Connected) {
            auto_login = true;
            pending_auto_login_ = false;
        }
    }
    notify_redraw();
    if (auto_login) {
        submit_login();
    }
}

void PokerUI::lobby_room_step(const int delta)
{
    {
        std::lock_guard lock(mtx_);
        if (state_.screen != UiScreen::Lobby || room_labels_.empty()) {
            return;
        }
        selected_room_index_ += delta;
        if (selected_room_index_ < 0) {
            selected_room_index_ = 0;
        } else if (selected_room_index_ >= static_cast<int>(room_labels_.size())) {
            selected_room_index_ = static_cast<int>(room_labels_.size()) - 1;
        }
    }
    notify_redraw();
}

bool PokerUI::text_input_has_focus() const
{
    return (player_name_input_ && player_name_input_->Focused())
        || (login_password_input_ && login_password_input_->Focused())
        || (login_host_input_ && login_host_input_->Focused())
        || (login_port_input_ && login_port_input_->Focused())
        || (create_name_input_ && create_name_input_->Focused())
        || (create_size_input_ && create_size_input_->Focused())
        || (raise_input_ && raise_input_->Focused());
}

void PokerUI::notify_redraw()
{
    if (screen_ != nullptr) {
        screen_->Post(Event::Custom);
    }
}

void PokerUI::sync_control_visibility()
{
    show_login_controls_ = state_.screen == UiScreen::Login;
    show_lobby_controls_ = state_.screen == UiScreen::Lobby;
    show_create_controls_ = show_lobby_controls_ && show_create_panel_;
    show_waiting_controls_ = state_.screen == UiScreen::WaitingRoom;
    show_waiting_start_ = show_waiting_controls_ && state_.is_room_host && state_.player_names.size() >= 2;
    show_game_controls_ = state_.screen == UiScreen::InGame;
    show_disconnected_controls_ = state_.screen == UiScreen::Disconnected;

    show_fold_ = false;
    show_check_ = false;
    show_call_ = false;
    show_raise_ = false;
    show_raise_presets_ = false;
    show_turn_actions_ = show_game_controls_ && state_.your_turn.has_value();
    if (state_.your_turn.has_value()) {
        show_fold_ = state_.has_action(poker::protocol::Action::Fold);
        show_check_ = state_.has_action(poker::protocol::Action::Check);
        show_call_ = state_.has_action(poker::protocol::Action::Call);
        show_raise_ = state_.has_action(poker::protocol::Action::Raise);
        show_raise_presets_ = show_raise_;
    }
    update_turn_button_labels();
}

void PokerUI::apply_pending_refocus()
{
    switch (pending_refocus_) {
    case PendingRefocus::RaiseInput:
        if (raise_input_) {
            raise_input_->TakeFocus();
        }
        break;
    case PendingRefocus::CreateNameInput:
        if (create_name_input_) {
            create_name_input_->TakeFocus();
        }
        break;
    case PendingRefocus::PlayerNameInput:
        if (player_name_input_) {
            player_name_input_->TakeFocus();
        }
        break;
    case PendingRefocus::PasswordInput:
        if (login_password_input_) {
            login_password_input_->TakeFocus();
        }
        break;
    case PendingRefocus::None:
        break;
    }
    pending_refocus_ = PendingRefocus::None;
}

void PokerUI::add_server_message(const poker::protocol::ServerMessage& msg)
{
    {
        std::lock_guard lock(mtx_);
        state_.apply_message(msg);

        if (std::holds_alternative<poker::protocol::Error>(msg)) {
            if (action_pending_ && saved_your_turn_.has_value()) {
                state_.your_turn = saved_your_turn_;
            }
            action_pending_ = false;
            saved_your_turn_.reset();
            if (state_.your_turn.has_value()) {
                pending_refocus_ = PendingRefocus::RaiseInput;
            } else if (show_create_panel_) {
                pending_refocus_ = PendingRefocus::CreateNameInput;
            } else if (state_.screen == UiScreen::Login) {
                pending_refocus_ = PendingRefocus::PasswordInput;
            }
        } else if (std::holds_alternative<poker::protocol::Welcome>(msg)) {
            save_client_settings(ClientSettings {
                state_.player_name,
                state_.server_host,
                state_.server_port,
                language_code(current_language()) });
            app_->list_rooms();
        } else if (std::holds_alternative<poker::protocol::GameStateUpdate>(msg)) {
            if (action_pending_) {
                state_.your_turn.reset();
                action_pending_ = false;
                saved_your_turn_.reset();
            }
        } else if (std::holds_alternative<poker::protocol::YourTurn>(msg)) {
            action_pending_ = false;
            saved_your_turn_.reset();
            raise_amount_ = std::to_string(state_.your_turn->min_amount);
            ring_bell_ = true;
        } else if (std::holds_alternative<poker::protocol::LeftRoom>(msg)) {
            action_pending_ = false;
            saved_your_turn_.reset();
        }

        rebuild_room_labels();
        sync_control_visibility();
    }
    notify_redraw();
}

void PokerUI::send_action(const poker::protocol::Action action, const std::optional<uint32_t> amount)
{
    {
        std::lock_guard lock(mtx_);
        if (!state_.your_turn.has_value()) {
            state_.append_log(tr(Msg::ActionIgnored));
            return;
        }
        if (!state_.has_action(action)) {
            state_.append_log(tr(Msg::ActionNotAvailable, trim_label(tr_action(action))));
            return;
        }
        saved_your_turn_ = state_.your_turn;
        action_pending_ = true;
        state_.your_turn.reset();
        sync_control_visibility();
    }

    app_->send_player_action(action, amount);
}

void PokerUI::send_preset_raise(uint32_t amount)
{
    {
        std::lock_guard lock(mtx_);
        if (!state_.your_turn.has_value() || !state_.has_action(poker::protocol::Action::Raise)) {
            return;
        }
        const auto& turn = *state_.your_turn;
        amount = std::clamp(amount, turn.min_amount, turn.max_amount);
        raise_amount_ = std::to_string(amount);
    }
    send_action(poker::protocol::Action::Raise, amount);
}

void PokerUI::rebuild_room_labels()
{
    room_labels_.clear();
    room_labels_.reserve(state_.rooms.size());
    for (const auto& room : state_.rooms) {
        room_labels_.push_back(room_label(room));
    }

    if (room_labels_.empty()) {
        selected_room_index_ = 0;
    } else if (selected_room_index_ >= static_cast<int>(room_labels_.size())) {
        selected_room_index_ = static_cast<int>(room_labels_.size()) - 1;
    }
}

void PokerUI::show_create_panel(const bool show)
{
    {
        std::lock_guard lock(mtx_);
        show_create_panel_ = show;
        sync_control_visibility();
        if (show) {
            pending_refocus_ = PendingRefocus::CreateNameInput;
        }
    }
    notify_redraw();
}

void PokerUI::submit_login()
{
    if (!app_->is_connected()) {
        std::lock_guard lock(mtx_);
        state_.append_log(tr(Msg::NotConnected));
        notify_redraw();
        return;
    }

    std::string name;
    std::string password;
    std::string host;
    std::string port;
    bool endpoint_changed = false;
    {
        std::lock_guard lock(mtx_);
        name = player_name_input_value_;
        password = login_password_value_;
        host = login_host_value_;
        port = login_port_value_;
        const auto trim_start = name.find_first_not_of(" \t\r\n");
        if (trim_start == std::string::npos) {
            state_.append_log(tr(Msg::EnterPlayerName));
            pending_refocus_ = PendingRefocus::PlayerNameInput;
            notify_redraw();
            return;
        }
        const auto trim_end = name.find_last_not_of(" \t\r\n");
        name = name.substr(trim_start, trim_end - trim_start + 1);

        if (!poker::auth::is_valid_username(name)) {
            state_.append_log(tr(Msg::NameLengthInvalid));
            pending_refocus_ = PendingRefocus::PlayerNameInput;
            notify_redraw();
            return;
        }

        if (password.empty()) {
            state_.append_log(tr(Msg::EnterPassword));
            pending_refocus_ = PendingRefocus::PasswordInput;
            notify_redraw();
            return;
        }

        if (!poker::auth::is_valid_password(password)) {
            state_.append_log(tr(Msg::PasswordRules));
            pending_refocus_ = PendingRefocus::PasswordInput;
            notify_redraw();
            return;
        }

        endpoint_changed = host != state_.server_host || port != state_.server_port;
        state_.player_name = name;
        state_.server_host = host;
        state_.server_port = port;
    }

    if (endpoint_changed) {
        pending_auto_login_ = true;
        app_->reconnect_to(host, port);
        {
            std::lock_guard lock(mtx_);
            state_.append_log(tr(Msg::JoiningAs, name));
            state_.status_message = tr(Msg::ConnectingTo, host + ":" + port);
        }
        notify_redraw();
        return;
    }

    if (!app_->is_connected()) {
        std::lock_guard lock(mtx_);
        state_.append_log(tr(Msg::NotConnected));
        notify_redraw();
        return;
    }

    app_->send_login(name, password);

    save_client_settings(ClientSettings {
        name,
        host,
        port,
        language_code(current_language()) });

    {
        std::lock_guard lock(mtx_);
        state_.append_log(tr(Msg::JoiningAs, name));
        state_.status_message = tr(Msg::Authenticating);
    }
    notify_redraw();
}

void PokerUI::submit_create_room()
{
    uint32_t max_players = 0;
    const auto [ptr, ec] = std::from_chars(
        create_max_players_.data(),
        create_max_players_.data() + create_max_players_.size(),
        max_players);

    {
        std::lock_guard lock(mtx_);

        if (ec != std::errc {} || ptr != create_max_players_.data() + create_max_players_.size()) {
            state_.append_log(tr(Msg::InvalidMaxPlayers));
            pending_refocus_ = PendingRefocus::CreateNameInput;
            notify_redraw();
            return;
        }

        if (create_room_name_.empty() || max_players < 2 || max_players > 255) {
            state_.append_log(tr(Msg::RoomNameRequired));
            pending_refocus_ = PendingRefocus::CreateNameInput;
            notify_redraw();
            return;
        }
    }

    app_->create_room(create_room_name_, static_cast<uint8_t>(max_players));

    {
        std::lock_guard lock(mtx_);
        show_create_panel_ = false;
        create_room_name_.clear();
        sync_control_visibility();
    }
    notify_redraw();
}

void PokerUI::submit_raise()
{
    std::optional<uint32_t> amount;
    {
        std::lock_guard lock(mtx_);

        if (!state_.your_turn.has_value()) {
            return;
        }

        uint32_t parsed = 0;
        const auto [ptr, parse_ec] = std::from_chars(
            raise_amount_.data(),
            raise_amount_.data() + raise_amount_.size(),
            parsed);

        if (parse_ec != std::errc {} || ptr != raise_amount_.data() + raise_amount_.size()) {
            state_.append_log(tr(Msg::InvalidRaiseAmount));
            pending_refocus_ = PendingRefocus::RaiseInput;
            notify_redraw();
            return;
        }

        const auto& turn = *state_.your_turn;
        if (parsed < turn.min_amount || parsed > turn.max_amount) {
            state_.append_log(tr(Msg::RaiseBetween, std::to_string(turn.min_amount), std::to_string(turn.max_amount)));
            pending_refocus_ = PendingRefocus::RaiseInput;
            notify_redraw();
            return;
        }

        amount = parsed;
    }

    send_action(poker::protocol::Action::Raise, amount);
    notify_redraw();
}

Element PokerUI::render_header() const
{
    std::string title = tr(Msg::AppTitle);
    if (!state_.player_name.empty()) {
        title += "  |  " + state_.player_name;
        title += "  |  " + std::to_string(state_.chips) + tr(Msg::PlayerChips);
    }
    if (state_.room_id.has_value()) {
        title += "  |  " + tr(Msg::HeaderRoom, std::to_string(*state_.room_id));
    }

    return vbox({
        hbox({
            text(title) | bold | color(Color::Yellow),
            filler(),
            text(connection_status_label(state_.connection)) | dim,
        }),
        hbox({
            text(tr(Msg::LanguageHint, language_display_name(current_language()))) | dim,
            filler(),
            text(state_.status_message) | dim,
        }),
    });
}

Element PokerUI::render_login() const
{
    const bool ready = state_.connection == ConnectionStatus::Connected;
    return vbox(
        text(tr(Msg::EnterYourName)) | bold,
        separator(),
        text(tr(Msg::NameLabel) + " / " + tr(Msg::PasswordLabel) + " / " + tr(Msg::HostLabel) + " / " + tr(Msg::PortLabel) + " — " + tr(Msg::TypeBelow)) | dim,
        text(tr(Msg::PressJoinWhenReady)) | dim,
        text(tr(Msg::PasswordRules)) | dim,
        text(ready ? tr(Msg::ConnectedTo, state_.server_host + ":" + state_.server_port)
                   : tr(Msg::WaitingForConnection))
            | dim);
}

Element PokerUI::render_disconnected() const
{
    return vbox(
        text(tr(Msg::ConnectionLostTitle)) | bold | color(Color::Red),
        separator(),
        text(tr(Msg::ConnectionLostDetail)) | dim,
        text(tr(Msg::PressReconnect)) | dim);
}

Element PokerUI::render_lobby() const
{
    Elements room_lines;
    if (room_labels_.empty()) {
        room_lines.push_back(text(tr(Msg::NoRoomsYet)) | dim);
    } else {
        for (size_t i = 0; i < room_labels_.size(); ++i) {
            const bool selected = static_cast<int>(i) == selected_room_index_;
            room_lines.push_back(text((selected ? "> " : "  ") + room_labels_[i])
                | (selected ? color(Color::Yellow) | bold : nothing));
        }
    }

    Elements create_panel;
    if (show_create_panel_) {
        create_panel = {
            separator(),
            text(tr(Msg::CreateRoom)) | bold,
            hbox(text(" " + tr(Msg::NameLabel) + " "), text(create_room_name_.empty() ? tr(Msg::TypeBelow) : create_room_name_)),
            hbox(text(" " + tr(Msg::MaxPlayersLabel) + " "), text(create_max_players_)),
            text(tr(Msg::PressCreateConfirm)) | dim,
        };
    }

    return vbox(
        text(tr(Msg::Lobby)) | bold,
        separator(),
        vbox(std::move(room_lines)) | border,
        vbox(std::move(create_panel)),
        separator(),
        text(tr(Msg::BlindsInfo,
            std::to_string(poker::SMALL_BLIND),
            std::to_string(poker::BIG_BLIND),
            std::to_string(state_.chips)))
            | dim);
}

Element PokerUI::render_waiting_room() const
{
    Elements seats;
    for (const auto& name : state_.player_names) {
        const bool is_host = name == state_.room_host_name;
        seats.push_back(text(std::string("  (.) ") + name + (is_host ? tr(Msg::HostTag) : "")));
    }

    std::string hint;
    if (state_.is_room_host) {
        hint = state_.player_names.size() >= 2 ? tr(Msg::PressStartWhenReady) : tr(Msg::NeedTwoPlayers);
    } else {
        hint = tr(Msg::WaitingForHostStart, state_.room_host_name);
    }

    const std::string count = tr(Msg::PlayersCount,
        std::to_string(state_.player_names.size()),
        std::to_string(state_.room_max_players));

    return vbox(
        text(tr(Msg::WaitingForPlayers)) | bold,
        separator(),
        text(tr(Msg::PlayersAtTable)),
        vbox(std::move(seats)) | border,
        text(count) | center,
        text(hint) | dim | center,
        text(tr(Msg::AutoStartWhenFull)) | dim | center);
}

Element PokerUI::render_hand_result_banner() const
{
    if (!state_.last_hand_result.has_value()) {
        return text("");
    }
    return vbox({
        text(hand_result_text(*state_.last_hand_result, state_.game_state)) | color(Color::Cyan) | bold | center,
        text(tr(Msg::NextHandSoon)) | dim | center,
    });
}

Element PokerUI::render_game_table() const
{
    if (!state_.game_state.has_value()) {
        return text(tr(Msg::WaitingForGameState)) | dim;
    }

    const auto& game = *state_.game_state;
    const auto seats = !game.players.empty()
        ? render_table_seats(game.players, state_.player_name, game.active_player_name)
        : render_table_seats({}, state_.player_name, game.active_player_name);

    const std::string hint = state_.hand_hint();
    Elements body = {
        text(tr(Msg::PhasePrefix, tr_phase(game.phase))) | bold,
        render_hand_result_banner(),
        separator(),
        hbox(text(tr(Msg::PotLabel) + " "), text(std::to_string(game.total_pot)) | color(Color::Yellow) | bold),
        hbox(text(tr(Msg::TableBetLabel) + " "), text(std::to_string(game.current_bet)) | dim),
        separator(),
        text(tr(Msg::CommunityCards)),
        render_card_row(game.general_cards, 5),
        separator(),
        text(tr(Msg::YourHand)),
        render_card_row(game.your_cards, 2),
        hint.empty() ? text("") : text(tr(Msg::HandPrefix) + " " + hint) | dim,
        separator(),
        text(tr(Msg::TableLabel)),
        seats,
    };

    return vbox(std::move(body)) | bgcolor(ui_theme::panel_background()) | border;
}

Element PokerUI::render_body() const
{
    switch (state_.screen) {
    case UiScreen::Login:
        return render_login();
    case UiScreen::Lobby:
        return render_lobby();
    case UiScreen::WaitingRoom:
        return render_waiting_room();
    case UiScreen::InGame:
        return render_game_table();
    case UiScreen::Disconnected:
        return render_disconnected();
    }
    return text("");
}

Element PokerUI::render_log_panel() const
{
    Elements lines;
    const size_t visible_lines = 8;
    const size_t total = state_.log.size();
    const size_t max_offset = total > visible_lines ? total - visible_lines : 0;
    const size_t offset = static_cast<size_t>(std::min(log_scroll_offset_, static_cast<int>(max_offset)));
    const size_t end = total > offset ? total - offset : 0;
    const size_t start = end > visible_lines ? end - visible_lines : 0;
    for (size_t i = start; i < end; ++i) {
        lines.push_back(text(state_.log[i]) | dim);
    }
    if (lines.empty()) {
        lines.push_back(text(tr(Msg::EventLog)) | dim);
    }
    return vbox(std::move(lines)) | border | size(HEIGHT, EQUAL, 10);
}

Element PokerUI::render_status_bar() const
{
    if (state_.your_turn.has_value()) {
        const auto& turn = *state_.your_turn;
        std::ostringstream ss;
        ss << tr(Msg::YourTurnBar, std::to_string(turn.your_chips));
        if (turn.to_call > 0) {
            ss << tr(Msg::ToCallSuffix, std::to_string(turn.to_call));
        }
        ss << tr(Msg::RaiseRange, std::to_string(turn.min_amount), std::to_string(turn.max_amount));
        return text(ss.str()) | color(Color::Yellow) | bold | center;
    }
    return text("");
}

bool PokerUI::handle_hotkeys(const Event event)
{
    if (text_input_has_focus()) {
        return false;
    }

    std::optional<poker::protocol::Action> action;
    bool toggle_lang = false;

    {
        std::lock_guard lock(mtx_);

        if (event == Event::Character('j') || event == Event::ArrowDown) {
            if (state_.screen == UiScreen::Lobby && !room_labels_.empty()) {
                selected_room_index_
                    = std::min(static_cast<int>(room_labels_.size()) - 1, selected_room_index_ + 1);
                return true;
            }
        }
        if (event == Event::Character('k') || event == Event::ArrowUp) {
            if (state_.screen == UiScreen::Lobby && !room_labels_.empty()) {
                selected_room_index_ = std::max(0, selected_room_index_ - 1);
                return true;
            }
        }
        if (event == Event::Character('[')) {
            const size_t max_offset = state_.log.size() > 8 ? state_.log.size() - 8 : 0;
            log_scroll_offset_ = std::min(log_scroll_offset_ + 1, static_cast<int>(max_offset));
            return true;
        }
        if (event == Event::Character(']')) {
            log_scroll_offset_ = std::max(0, log_scroll_offset_ - 1);
            return true;
        }

        if (event == Event::Character('l') || event == Event::Character('L')) {
            toggle_lang = true;
        } else if (state_.screen == UiScreen::InGame && state_.your_turn.has_value()) {
            if (event == Event::Character('f') && state_.has_action(poker::protocol::Action::Fold)) {
                action = poker::protocol::Action::Fold;
            } else if (event == Event::Character('c')) {
                if (state_.has_action(poker::protocol::Action::Check)) {
                    action = poker::protocol::Action::Check;
                } else if (state_.has_action(poker::protocol::Action::Call)) {
                    action = poker::protocol::Action::Call;
                }
            } else if (event == Event::Character('r') && state_.has_action(poker::protocol::Action::Raise)) {
                pending_refocus_ = PendingRefocus::RaiseInput;
                return true;
            }
        }
    }

    if (toggle_lang) {
        toggle_language();
        return true;
    }

    if (action.has_value()) {
        send_action(*action);
        return true;
    }

    return false;
}

Element PokerUI::render_ui() const
{
    return vbox(
               render_header(),
               separator(),
               render_body() | flex,
               separator(),
               render_status_bar(),
               render_log_panel())
        | bgcolor(ui_theme::panel_background())
        | border;
}

void PokerUI::run()
{
    auto screen = ScreenInteractive::Fullscreen();
    screen_ = &screen;

    refresh_button_labels();

    auto enter_btn = ui_theme::make_button(&btn_.join, [this] { submit_login(); });
    auto login_quit_btn = ui_theme::make_button(&btn_.quit, [this, &screen] {
        app_->quit();
        screen.ExitLoopClosure()();
    });

    player_name_input_ = ui_theme::make_input(&player_name_input_value_, "username");
    login_password_input_ = ui_theme::make_password_input(
        &login_password_value_, "password", &login_password_mode_);
    login_host_input_ = ui_theme::make_input(&login_host_value_, "host");
    login_port_input_ = ui_theme::make_input(&login_port_value_, "port");
    auto login_lang_btn = ui_theme::make_button(&btn_.lang, [this] { toggle_language(); });
    auto login_controls = Container::Vertical(Components {
        player_name_input_,
        login_password_input_,
        login_host_input_,
        login_port_input_,
        Container::Horizontal(Components { enter_btn, login_lang_btn, login_quit_btn }),
    });

    auto refresh_btn = ui_theme::make_button(&btn_.refresh, [this] { app_->list_rooms(); });
    auto create_btn = ui_theme::make_button(&btn_.create, [this] {
        std::lock_guard lock(mtx_);
        show_create_panel_ = !show_create_panel_;
        sync_control_visibility();
        if (show_create_panel_) {
            pending_refocus_ = PendingRefocus::CreateNameInput;
        }
        notify_redraw();
    });
    auto join_room_btn = ui_theme::make_button(&btn_.join, [this] {
        std::lock_guard lock(mtx_);
        if (selected_room_index_ >= 0 && selected_room_index_ < static_cast<int>(state_.rooms.size())) {
            const auto& room = state_.rooms[static_cast<size_t>(selected_room_index_)];
            if (room.in_game) {
                state_.append_log(tr(Msg::CannotJoinInGame));
                notify_redraw();
                return;
            }
            app_->join_room(room.room_id);
        } else {
            state_.append_log(tr(Msg::SelectRoom));
            notify_redraw();
        }
    });
    auto waiting_leave_btn = ui_theme::make_button(&btn_.leave, [this] { app_->leave_room(); });
    auto game_leave_btn = ui_theme::make_button(&btn_.leave, [this] { app_->leave_room(); });
    auto start_btn = ui_theme::make_button(&btn_.start, [this] { app_->start_game(); });
    auto refresh_btn_waiting = ui_theme::make_button(&btn_.refresh, [this] { app_->list_rooms(); });
    auto lobby_quit_btn = ui_theme::make_button(&btn_.quit, [this, &screen] {
        app_->quit();
        screen.ExitLoopClosure()();
    });
    auto waiting_quit_btn = ui_theme::make_button(&btn_.quit, [this, &screen] {
        app_->quit();
        screen.ExitLoopClosure()();
    });
    auto game_quit_btn = ui_theme::make_button(&btn_.quit, [this, &screen] {
        app_->quit();
        screen.ExitLoopClosure()();
    });
    auto reconnect_btn = ui_theme::make_button(&btn_.reconnect, [this] {
        {
            std::lock_guard lock(mtx_);
            pending_auto_login_ = !state_.player_name.empty() && !login_password_value_.empty();
            state_.reset_play_session();
            state_.connection = ConnectionStatus::Connecting;
            state_.status_message = tr(Msg::ConnectingTo, state_.server_host + ":" + state_.server_port);
            state_.screen = UiScreen::Login;
            sync_control_visibility();
        }
        notify_redraw();
        app_->reconnect();
    });
    auto disconnected_quit_btn = ui_theme::make_button(&btn_.quit, [this, &screen] {
        app_->quit();
        screen.ExitLoopClosure()();
    });

    auto confirm_create_btn = ui_theme::make_button(&btn_.confirm, [this] { submit_create_room(); });
    auto cancel_create_btn = ui_theme::make_button(&btn_.cancel, [this] { show_create_panel(false); });

    create_name_input_ = ui_theme::make_input(&create_room_name_, "room name");
    create_size_input_ = ui_theme::make_input(&create_max_players_, "max players");

    auto fold_btn = ui_theme::make_button(&btn_.fold, [this] { send_action(poker::protocol::Action::Fold); });
    auto check_btn = ui_theme::make_button(&btn_.check, [this] { send_action(poker::protocol::Action::Check); });
    auto call_btn = ui_theme::make_button(&btn_.call, [this] { send_action(poker::protocol::Action::Call); });
    auto raise_btn = ui_theme::make_button(&btn_.raise, [this] { submit_raise(); });
    raise_input_ = ui_theme::make_input(&raise_amount_, "amount");

    auto min_raise_btn = ui_theme::make_button(&btn_.min, [this] {
        uint32_t amount = 0;
        {
            std::lock_guard lock(mtx_);
            if (state_.your_turn.has_value()) {
                amount = state_.your_turn->min_amount;
            }
        }
        send_preset_raise(amount);
    });
    auto half_raise_btn = ui_theme::make_button(&btn_.half, [this] {
        uint32_t amount = 0;
        {
            std::lock_guard lock(mtx_);
            if (state_.your_turn.has_value() && state_.game_state.has_value()) {
                const auto& turn = *state_.your_turn;
                const uint32_t pot_target = state_.game_state->total_pot + turn.to_call;
                amount = std::max(turn.min_amount, std::min(turn.max_amount, pot_target / 2));
            }
        }
        send_preset_raise(amount);
    });
    auto pot_raise_btn = ui_theme::make_button(&btn_.pot, [this] {
        uint32_t amount = 0;
        {
            std::lock_guard lock(mtx_);
            if (state_.your_turn.has_value() && state_.game_state.has_value()) {
                const auto& turn = *state_.your_turn;
                amount = std::max(turn.min_amount,
                    std::min(turn.max_amount, state_.game_state->total_pot + turn.to_call));
            }
        }
        send_preset_raise(amount);
    });
    auto allin_raise_btn = ui_theme::make_button(&btn_.allin, [this] {
        uint32_t amount = 0;
        {
            std::lock_guard lock(mtx_);
            if (state_.your_turn.has_value()) {
                amount = state_.your_turn->max_amount;
            }
        }
        send_preset_raise(amount);
    });

    auto room_up_btn = ui_theme::make_button(&btn_.room_up, [this] { lobby_room_step(-1); });
    auto room_down_btn = ui_theme::make_button(&btn_.room_down, [this] { lobby_room_step(1); });
    auto lobby_lang_btn = ui_theme::make_button(&btn_.lang, [this] { toggle_language(); });
    auto lobby_controls = Container::Horizontal(Components {
        room_up_btn,
        room_down_btn,
        refresh_btn,
        create_btn,
        join_room_btn,
        lobby_lang_btn,
        lobby_quit_btn,
    });
    auto create_controls = Container::Vertical(Components {
        create_name_input_,
        create_size_input_,
        Container::Horizontal(Components { confirm_create_btn, cancel_create_btn }),
    });
    auto waiting_start_controls = Container::Horizontal(Components { start_btn });
    auto waiting_controls = Container::Horizontal(Components { waiting_leave_btn, refresh_btn_waiting, waiting_quit_btn });
    auto raise_presets = Container::Horizontal(Components { min_raise_btn, half_raise_btn, pot_raise_btn, allin_raise_btn });
    auto turn_controls = Container::Horizontal(Components {
        Maybe(fold_btn, &show_fold_),
        Maybe(check_btn, &show_check_),
        Maybe(call_btn, &show_call_),
        Maybe(raise_input_, &show_raise_),
        Maybe(raise_btn, &show_raise_),
    });
    auto game_bar = Container::Horizontal(Components { game_leave_btn, game_quit_btn });
    auto disconnected_controls = Container::Horizontal(Components { reconnect_btn, disconnected_quit_btn });

    auto root = Container::Vertical(Components {
        Maybe(login_controls, &show_login_controls_),
        Maybe(lobby_controls, &show_lobby_controls_),
        Maybe(create_controls, &show_create_controls_),
        Maybe(waiting_start_controls, &show_waiting_start_),
        Maybe(waiting_controls, &show_waiting_controls_),
        Maybe(raise_presets, &show_raise_presets_),
        Maybe(turn_controls, &show_turn_actions_),
        Maybe(game_bar, &show_game_controls_),
        Maybe(disconnected_controls, &show_disconnected_controls_),
    });

    root = CatchEvent(root, [this](const Event event) {
        if (event == Event::Return) {
            bool on_login = false;
            {
                std::lock_guard lock(mtx_);
                on_login = state_.screen == UiScreen::Login;
            }
            if (on_login) {
                submit_login();
                return true;
            }
        }
        if (handle_hotkeys(event)) {
            notify_redraw();
            return true;
        }
        return false;
    });

    {
        std::lock_guard lock(mtx_);
        sync_control_visibility();
        pending_refocus_ = PendingRefocus::PlayerNameInput;
    }

    auto renderer = Renderer(root, [this, root] {
        std::lock_guard lock(mtx_);
        apply_pending_refocus();

        if (ring_bell_) {
            ring_bell_ = false;
            std::cout << '\a' << std::flush;
        }

        return vbox(
                   render_ui(),
                   separator(),
                   text(tr(Msg::Controls)) | dim,
                   root->Render(),
                   text(tr(Msg::HotkeyHint)) | dim)
            | flex;
    });

    screen.Loop(renderer);
}

} // namespace poker::client
