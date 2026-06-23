#include "poker_ui.h"

#include "poker/card.h"
#include "poker/game_constants.h"

#include <charconv>
#include <ftxui_all.hpp>

#include <algorithm>
#include <optional>
#include <sstream>
#include <variant>

namespace poker::client {

using namespace ftxui;

namespace {

    Element colored_card(const poker::Card& card)
    {
        const std::string label = card.to_string();
        const char suit = label.back();
        const bool red = suit == 'H' || suit == 'D';
        const std::string top = "+" + std::string(label.size() + 2, '-');
        const std::string mid = "| " + label + " |";
        const std::string bot = top;

        Element body = vbox({ text(top), text(mid), text(bot) });
        return body | (red ? color(Color::Red) : color(Color::White)) | bold;
    }

    Element card_placeholder()
    {
        return vbox({
                   text("+---+"),
                   text("| ? |"),
                   text("+---+"),
               })
            | color(Color::GrayDark);
    }

    Element render_card_row(const std::vector<poker::Card>& cards, size_t slots)
    {
        Elements elements;
        for (size_t i = 0; i < slots; ++i) {
            if (i < cards.size()) {
                elements.push_back(colored_card(cards[i]));
            } else {
                elements.push_back(card_placeholder());
            }
            elements.push_back(text(" "));
        }
        return hbox(std::move(elements));
    }

    std::string room_label(const poker::protocol::RoomInfo& room)
    {
        std::ostringstream ss;
        ss << "Room " << room.room_id << ": " << room.room_name << "  ["
           << static_cast<int>(room.current_players) << "/"
           << static_cast<int>(room.max_players) << "]";
        if (room.in_game) {
            ss << "  (in game)";
        }
        return ss.str();
    }

} // namespace

PokerUI::PokerUI(std::shared_ptr<ClientApplication> app)
    : app_(std::move(app))
{
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
    show_turn_actions_ = show_game_controls_ && state_.your_turn.has_value();
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
                pending_refocus_ = PendingRefocus::PlayerNameInput;
            }
        } else if (std::holds_alternative<poker::protocol::Welcome>(msg)) {
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
            state_.append_log("Action ignored: not your turn");
            return;
        }
        saved_your_turn_ = state_.your_turn;
        action_pending_ = true;
        state_.your_turn.reset();
        sync_control_visibility();
    }

    app_->send_player_action(action, amount);
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

void PokerUI::submit_hello()
{
    std::string name;
    {
        std::lock_guard lock(mtx_);
        name = player_name_input_value_;
        const auto trim_start = name.find_first_not_of(" \t\r\n");
        if (trim_start == std::string::npos) {
            state_.append_log("Enter a player name");
            pending_refocus_ = PendingRefocus::PlayerNameInput;
            notify_redraw();
            return;
        }
        const auto trim_end = name.find_last_not_of(" \t\r\n");
        name = name.substr(trim_start, trim_end - trim_start + 1);

        if (name.empty() || name.size() > 32) {
            state_.append_log("Name must be 1-32 characters");
            pending_refocus_ = PendingRefocus::PlayerNameInput;
            notify_redraw();
            return;
        }
    }

    app_->send_hello(name);

    {
        std::lock_guard lock(mtx_);
        state_.append_log("Joining as " + name + "...");
        state_.status_message = "Connecting...";
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
            state_.append_log("Invalid max players value");
            pending_refocus_ = PendingRefocus::CreateNameInput;
            notify_redraw();
            return;
        }

        if (create_room_name_.empty() || max_players < 2 || max_players > 255) {
            state_.append_log("Room name required, max players 2-255");
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
            state_.append_log("Invalid raise amount");
            pending_refocus_ = PendingRefocus::RaiseInput;
            notify_redraw();
            return;
        }

        const auto& turn = *state_.your_turn;
        if (parsed < turn.min_amount || parsed > turn.max_amount) {
            state_.append_log("Raise must be between " + std::to_string(turn.min_amount) + " and "
                + std::to_string(turn.max_amount));
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
    std::string title = "ASCII Poker";
    if (!state_.player_name.empty()) {
        title += "  |  " + state_.player_name;
    }
    if (state_.room_id.has_value()) {
        title += "  |  Room " + std::to_string(*state_.room_id);
    }

    return hbox({
        text(title) | bold | color(Color::Yellow),
        filler(),
        text(state_.status_message) | dim,
    });
}

Element PokerUI::render_login() const
{
    return vbox(
        text("Enter your name") | bold,
        separator(),
        hbox(text(" Name: "), text(player_name_input_value_.empty() ? "(type below)" : player_name_input_value_)),
        separator(),
        text("Names must be unique. Press Enter on [Join] to connect.") | dim);
}

Element PokerUI::render_lobby() const
{
    Elements room_lines;
    if (room_labels_.empty()) {
        room_lines.push_back(text("No rooms yet. Refresh or create one.") | dim);
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
            text("Create room") | bold,
            hbox(text(" Name: "), text(create_room_name_.empty() ? "(type below)" : create_room_name_)),
            hbox(text(" Max players: "), text(create_max_players_)),
            text("Press Enter on [Create] to confirm") | dim,
        };
    }

    return vbox(
        text("Lobby") | bold,
        separator(),
        vbox(std::move(room_lines)) | border,
        vbox(std::move(create_panel)),
        separator(),
        text("Blinds: " + std::to_string(poker::SMALL_BLIND) + "/" + std::to_string(poker::BIG_BLIND)
            + "  |  Starting stack: " + std::to_string(poker::STARTING_CHIPS))
            | dim);
}

Element PokerUI::render_waiting_room() const
{
    Elements seats;
    for (const auto& name : state_.player_names) {
        const bool is_host = name == state_.room_host_name;
        seats.push_back(text(std::string("  (.) ") + name + (is_host ? " [host]" : "")));
    }

    std::string hint;
    if (state_.is_room_host) {
        if (state_.player_names.size() >= 2) {
            hint = "Press [Start] when ready.";
        } else {
            hint = "Need at least 2 players to start.";
        }
    } else {
        hint = "Waiting for " + state_.room_host_name + " to start.";
    }

    std::ostringstream count;
    count << static_cast<int>(state_.player_names.size()) << "/" << static_cast<int>(state_.room_max_players)
          << " players";

    return vbox(
        text("Waiting for players") | bold,
        separator(),
        text("Players at the table:"),
        vbox(std::move(seats)) | border,
        text(count.str()) | center,
        text(hint) | dim | center,
        text("Game also auto-starts when the room is full.") | dim | center);
}

Element PokerUI::render_game_table() const
{
    if (!state_.game_state.has_value()) {
        return text("Waiting for game state...") | dim;
    }

    const auto& game = *state_.game_state;

    Elements player_lines;
    for (const auto& name : state_.player_names) {
        const bool active = game.active_player_name == name;
        player_lines.push_back(
            text((active ? ">> " : "   ") + name) | (active ? color(Color::Yellow) | bold : nothing));
    }

    return vbox(
               text("Phase: " + phase_to_string(game.phase)) | bold,
               separator(),
               hbox(text("Pot: "), text(std::to_string(game.total_pot) + " ") | color(Color::Yellow) | bold),
               separator(),
               text("Community cards:"),
               render_card_row(game.general_cards, 5),
               separator(),
               text("Your hand:"),
               render_card_row(game.your_cards, 2),
               separator(),
               text("Players:"),
               vbox(std::move(player_lines)),
               state_.last_hand_result.has_value()
                   ? text("Last hand: " + std::to_string(state_.last_hand_result->pot_amount) + " chips") | dim
                   : text(""))
        | bgcolor(Color::Green);
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
    }
    return text("");
}

Element PokerUI::render_log_panel() const
{
    Elements lines;
    const size_t start = state_.log.size() > 8 ? state_.log.size() - 8 : 0;
    for (size_t i = start; i < state_.log.size(); ++i) {
        lines.push_back(text(state_.log[i]) | dim);
    }
    if (lines.empty()) {
        lines.push_back(text("Event log") | dim);
    }
    return vbox(std::move(lines)) | border | size(HEIGHT, EQUAL, 10);
}

Element PokerUI::render_status_bar() const
{
    if (state_.your_turn.has_value()) {
        const auto& turn = *state_.your_turn;
        std::ostringstream ss;
        ss << "YOUR TURN  |  Raise " << turn.min_amount << "-" << turn.max_amount;
        return text(ss.str()) | color(Color::Yellow) | bold | center;
    }
    return text("");
}

bool PokerUI::handle_lobby_navigation(const Event event)
{
    std::lock_guard lock(mtx_);
    if (state_.screen != UiScreen::Lobby || room_labels_.empty()) {
        return false;
    }

    if (event == Event::ArrowUp) {
        selected_room_index_ = std::max(0, selected_room_index_ - 1);
        return true;
    }
    if (event == Event::ArrowDown) {
        selected_room_index_
            = std::min(static_cast<int>(room_labels_.size()) - 1, selected_room_index_ + 1);
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
        | border;
}

void PokerUI::run()
{
    auto screen = ScreenInteractive::Fullscreen();
    screen_ = &screen;

    auto enter_btn = Button(" Join ", [this] { submit_hello(); });
    auto login_quit_btn = Button(" Quit ", [this, &screen] {
        app_->quit();
        screen.ExitLoopClosure()();
    });

    player_name_input_ = Input(&player_name_input_value_, "your name");
    auto login_controls = Container::Vertical(Components {
        player_name_input_,
        Container::Horizontal(Components { enter_btn, login_quit_btn }),
    });

    auto refresh_btn = Button(" Refresh ", [this] { app_->list_rooms(); });
    auto create_btn = Button(" Create ", [this] {
        std::lock_guard lock(mtx_);
        show_create_panel_ = !show_create_panel_;
        sync_control_visibility();
        if (show_create_panel_) {
            pending_refocus_ = PendingRefocus::CreateNameInput;
        }
        notify_redraw();
    });
    auto join_room_btn = Button(" Join ", [this] {
        std::lock_guard lock(mtx_);
        if (selected_room_index_ >= 0 && selected_room_index_ < static_cast<int>(state_.rooms.size())) {
            app_->join_room(state_.rooms[static_cast<size_t>(selected_room_index_)].room_id);
        } else {
            state_.append_log("Select a room to join");
            notify_redraw();
        }
    });
    auto leave_btn = Button(" Leave ", [this] { app_->leave_room(); });
    auto start_btn = Button(" Start ", [this] { app_->start_game(); });
    auto refresh_btn_waiting = Button(" Refresh ", [this] { app_->list_rooms(); });
    auto lobby_quit_btn = Button(" Quit ", [this, &screen] {
        app_->quit();
        screen.ExitLoopClosure()();
    });

    auto confirm_create_btn = Button(" Confirm ", [this] { submit_create_room(); });
    auto cancel_create_btn = Button(" Cancel ", [this] { show_create_panel(false); });

    create_name_input_ = Input(&create_room_name_, "room name");
    auto create_size_input = Input(&create_max_players_, "max players");

    auto fold_btn = Button(" Fold ", [this] { send_action(poker::protocol::Action::Fold); });
    auto check_btn = Button(" Check ", [this] { send_action(poker::protocol::Action::Check); });
    auto call_btn = Button(" Call ", [this] { send_action(poker::protocol::Action::Call); });
    auto raise_btn = Button(" Raise ", [this] { submit_raise(); });
    raise_input_ = Input(&raise_amount_, "amount");

    auto lobby_controls = Container::Horizontal(Components { refresh_btn, create_btn, join_room_btn, lobby_quit_btn });
    auto create_controls = Container::Vertical(Components {
        create_name_input_,
        create_size_input,
        Container::Horizontal(Components { confirm_create_btn, cancel_create_btn }),
    });
    auto waiting_start_controls = Container::Horizontal(Components { start_btn });
    auto waiting_controls = Container::Horizontal(Components { leave_btn, refresh_btn_waiting, lobby_quit_btn });
    auto turn_controls = Container::Horizontal(Components { fold_btn, check_btn, call_btn, raise_input_, raise_btn });
    auto game_bar = Container::Horizontal(Components { leave_btn, lobby_quit_btn });

    auto root = Container::Vertical(Components {
        Maybe(login_controls, &show_login_controls_),
        Maybe(lobby_controls, &show_lobby_controls_),
        Maybe(create_controls, &show_create_controls_),
        Maybe(waiting_start_controls, &show_waiting_start_),
        Maybe(waiting_controls, &show_waiting_controls_),
        Maybe(turn_controls, &show_turn_actions_),
        Maybe(game_bar, &show_game_controls_),
    });

    root = CatchEvent(root, [this](const Event event) {
        if (event == Event::Return) {
            bool on_login = false;
            {
                std::lock_guard lock(mtx_);
                on_login = state_.screen == UiScreen::Login;
            }
            if (on_login) {
                submit_hello();
                return true;
            }
        }
        if (handle_lobby_navigation(event)) {
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
        apply_pending_refocus();

        std::lock_guard lock(mtx_);
        return vbox(
                   render_ui(),
                   separator(),
                   text("Controls:") | dim,
                   root->Render(),
                   text("Lobby: Up/Down select room") | dim)
            | flex;
    });

    screen.Loop(renderer);
}

} // namespace poker::client
