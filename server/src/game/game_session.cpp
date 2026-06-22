#include "game_session.h"
#include <spdlog/spdlog.h>

GameSession::GameSession(std::vector<std::shared_ptr<IPlayer>> players, uint64_t room_id)
    : players_(std::move(players))
    , room_id_(room_id)
    , phase_(poker::protocol::GamePhase::PreFlop)
{
    for (const auto& ip : players_) {
        // TODO
    }
    spdlog::info("GameSession created for room {} with {} players", room_id_, players_.size());
}

void GameSession::start()
{
    deck_.shuffle();

    for (const auto& player : players_) {
        auto hand = deck_.deal(2);
        hands_[player.get()] = std::move(hand);
        chips_[player.get()] = 1000;
        round_bets_[player.get()] = 0;
        spdlog::info("Player '{}' received hand: {}, {}",
            player->get_name(),
            hands_[player.get()][0].to_string(),
            hands_[player.get()][1].to_string());
    }

    dealer_index_ = 0;
    phase_ = poker::protocol::GamePhase::PreFlop;
    broadcast_state();
    ask_current_player();
}

void GameSession::next_phase()
{
    current_bet_ = 0;
    for (auto& [player, bet] : round_bets_)
        bet = 0;

    switch (phase_) {
    case poker::protocol::GamePhase::PreFlop: {
        auto flop = deck_.deal(3);
        community_cards_.insert(community_cards_.end(), flop.begin(), flop.end());
        phase_ = poker::protocol::GamePhase::Flop;
    } break;
    case poker::protocol::GamePhase::Flop: {
        auto turn = deck_.deal(1);
        community_cards_.push_back(turn[0]);
        phase_ = poker::protocol::GamePhase::Turn;
    } break;
    case poker::protocol::GamePhase::Turn: {
        auto river = deck_.deal(1);
        community_cards_.push_back(river[0]);
        phase_ = poker::protocol::GamePhase::River;
    } break;
    case poker::protocol::GamePhase::River:
        phase_ = poker::protocol::GamePhase::Showdown;
        spdlog::info("Showdown!");
        // TODO: Who win?!
        broadcast_state();
        return;
    default:
        return;
    }

    current_player_index_ = find_next_active_player();
    broadcast_state();
    ask_current_player();
}

void GameSession::apply_action(std::shared_ptr<Session> session, poker::protocol::Action action, std::optional<uint32_t> amount)
{
    IPlayer* current = get_player(session.get());
    if (!current) {
        spdlog::error("Session not mapped to any player");
        return;
    }

    if (players_[current_player_index_].get() != current) {
        current->send_message(poker::protocol::ServerMessage {
            poker::protocol::Error { 5, "Not your turn" } });
        return;
    }

    uint32_t& chips = chips_[current];
    uint32_t& bet = round_bets_[current];

    switch (action) {
    case poker::protocol::Action::Fold:
        folded_players_.insert(current);
        spdlog::info("Player '{}' folds", current->get_name());
        break;
    case poker::protocol::Action::Check:
        if (current_bet_ != 0) {
            current->send_message(poker::protocol::ServerMessage {
                poker::protocol::Error { 6, "Cannot check – there is a bet" } });
            return;
        }
        break;
    case poker::protocol::Action::Call: {
        uint32_t to_call = current_bet_ - bet;
        if (to_call > chips)
            to_call = chips; // all-in
        chips -= to_call;
        pot_ += to_call;
        bet += to_call;
    } break;
    case poker::protocol::Action::Raise: {
        if (!amount || *amount <= current_bet_) {
            current->send_message(poker::protocol::ServerMessage {
                poker::protocol::Error { 7, "Invalid raise amount" } });
            return;
        }
        uint32_t raise_amount = *amount - current_bet_;
        if (raise_amount > chips) {
            current->send_message(poker::protocol::ServerMessage {
                poker::protocol::Error { 8, "Not enough chips" } });
            return;
        }
        chips -= raise_amount;
        pot_ += raise_amount;
        bet += raise_amount;
        current_bet_ = *amount;
    } break;
    }

    advance_to_next_player();
    if (is_round_complete()) {
        next_phase();
    } else {
        ask_current_player();
    }
}

void GameSession::broadcast_state()
{
    for (const auto& player : players_) {
        poker::protocol::GameStateUpdate update;
        update.general_cards = community_cards_;
        update.your_cards = hands_[player.get()];
        update.phase = phase_;
        update.active_player_name = players_[current_player_index_]->get_name();
        update.total_pot = pot_;
        player->send_message(poker::protocol::ServerMessage { update });
    }
}

IPlayer* GameSession::get_player(Session* session) const
{
    auto it = session_map_.find(session);
    return it != session_map_.end() ? it->second : nullptr;
}

size_t GameSession::find_next_active_player() const
{
    size_t idx = (current_player_index_ + 1) % players_.size();
    while (folded_players_.count(players_[idx].get()) > 0 && idx != current_player_index_) {
        idx = (idx + 1) % players_.size();
    }
    return idx;
}

bool GameSession::is_round_complete() const
{
    size_t active_count = 0;
    for (const auto& p : players_) {
        if (folded_players_.count(p.get()) == 0)
            active_count++;
    }
    if (active_count == 1)
        return true;

    for (const auto& p : players_) {
        if (folded_players_.count(p.get()))
            continue;
        if (round_bets_.at(p.get()) != current_bet_)
            return false;
    }
    return true;
}

void GameSession::ask_current_player()
{
    IPlayer* current = players_[current_player_index_].get();
    poker::protocol::YourTurn turn;
    turn.available_actions = { poker::protocol::Action::Fold, poker::protocol::Action::Call, poker::protocol::Action::Raise };
    if (current_bet_ == 0)
        turn.available_actions.push_back(poker::protocol::Action::Check);
    turn.min_amount = current_bet_ + 1;
    turn.max_amount = chips_[current];
    current->send_message(poker::protocol::ServerMessage { turn });
}

void GameSession::advance_to_next_player()
{
    current_player_index_ = find_next_active_player();
}