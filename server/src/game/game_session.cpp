#include "game_session.h"

#include <poker/hand_evaluator.h>

#include <spdlog/spdlog.h>

#include <algorithm>

GameSession::GameSession(
    std::vector<std::shared_ptr<IPlayer>> players,
    std::unordered_map<Session*, IPlayer*> session_map,
    uint64_t room_id)
    : players_(std::move(players))
    , session_map_(std::move(session_map))
    , room_id_(room_id)
    , phase_(poker::protocol::GamePhase::PreFlop)
{
    spdlog::info("GameSession created for room {} with {} players", room_id_, players_.size());
}

void GameSession::start()
{
    deck_ = poker::Deck {};
    deck_.shuffle();
    community_cards_.clear();
    folded_players_.clear();
    all_in_players_.clear();
    pot_ = 0;
    current_bet_ = 0;

    for (const auto& player : players_) {
        if (chips_.find(player.get()) == chips_.end()) {
            chips_[player.get()] = 1000;
        }
        auto hand = deck_.deal(2);
        hands_[player.get()] = std::move(hand);
        round_bets_[player.get()] = 0;
        spdlog::info("Player '{}' received hand: {}, {}",
            player->get_name(),
            hands_[player.get()][0].to_string(),
            hands_[player.get()][1].to_string());
    }

    current_player_index_ = (dealer_index_ + 1) % players_.size();
    phase_ = poker::protocol::GamePhase::PreFlop;
    broadcast_state();
    ask_current_player();
}

void GameSession::next_phase()
{
    if (count_active_players() == 1) {
        for (const auto& player : players_) {
            if (folded_players_.count(player.get()) == 0) {
                handle_single_winner(player.get());
                return;
            }
        }
    }

    current_bet_ = 0;
    for (auto& [player, bet] : round_bets_) {
        (void)player;
        bet = 0;
    }

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
        do_showdown();
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
        if (current_bet_ != bet) {
            current->send_message(poker::protocol::ServerMessage {
                poker::protocol::Error { 6, "Cannot check – there is a bet" } });
            return;
        }
        break;
    case poker::protocol::Action::Call: {
        uint32_t to_call = current_bet_ - bet;
        if (to_call == 0) {
            break;
        }
        if (to_call > chips) {
            to_call = chips;
        }
        chips -= to_call;
        pot_ += to_call;
        bet += to_call;
        mark_all_in_if_needed(current);
    } break;
    case poker::protocol::Action::Raise: {
        if (!amount || *amount <= current_bet_) {
            current->send_message(poker::protocol::ServerMessage {
                poker::protocol::Error { 7, "Invalid raise amount" } });
            return;
        }
        if (*amount <= bet) {
            current->send_message(poker::protocol::ServerMessage {
                poker::protocol::Error { 7, "Invalid raise amount" } });
            return;
        }
        const uint32_t to_put = *amount - bet;
        if (to_put > chips) {
            current->send_message(poker::protocol::ServerMessage {
                poker::protocol::Error { 8, "Not enough chips" } });
            return;
        }
        chips -= to_put;
        pot_ += to_put;
        bet = *amount;
        if (*amount > current_bet_) {
            current_bet_ = *amount;
        }
        mark_all_in_if_needed(current);
    } break;
    }

    if (count_active_players() == 1) {
        for (const auto& player : players_) {
            if (folded_players_.count(player.get()) == 0) {
                handle_single_winner(player.get());
                return;
            }
        }
    }

    advance_to_next_player();
    if (is_round_complete()) {
        next_phase();
    } else {
        ask_current_player();
    }
}

void GameSession::on_player_disconnect(std::shared_ptr<Session> session)
{
    IPlayer* player = get_player(session.get());
    if (!player || folded_players_.count(player) > 0) {
        return;
    }

    folded_players_.insert(player);
    spdlog::info("Player '{}' disconnected – auto-fold", player->get_name());

    if (count_active_players() == 1) {
        for (const auto& p : players_) {
            if (folded_players_.count(p.get()) == 0) {
                handle_single_winner(p.get());
                return;
            }
        }
    }

    if (players_[current_player_index_].get() == player) {
        advance_to_next_player();
        if (is_round_complete()) {
            next_phase();
        } else {
            ask_current_player();
        }
    }
}

void GameSession::broadcast_state()
{
    for (const auto& player : players_) {
        poker::protocol::GameStateUpdate update;
        update.general_cards = community_cards_;
        update.your_cards = hands_[player.get()];
        update.phase = phase_;
        if (can_player_act(players_[current_player_index_].get())) {
            update.active_player_name = players_[current_player_index_]->get_name();
        }
        update.total_pot = pot_;
        player->send_message(poker::protocol::ServerMessage { update });
    }
}

IPlayer* GameSession::get_player(Session* session) const
{
    auto it = session_map_.find(session);
    return it != session_map_.end() ? it->second : nullptr;
}

size_t GameSession::count_active_players() const
{
    size_t count = 0;
    for (const auto& player : players_) {
        if (folded_players_.count(player.get()) == 0) {
            ++count;
        }
    }
    return count;
}

size_t GameSession::find_next_active_player() const
{
    if (players_.empty()) {
        return 0;
    }

    size_t idx = (current_player_index_ + 1) % players_.size();
    while (!can_player_act(players_[idx].get()) && idx != current_player_index_) {
        idx = (idx + 1) % players_.size();
    }
    return idx;
}

bool GameSession::is_round_complete() const
{
    if (count_active_players() <= 1) {
        return true;
    }

    for (const auto& player : players_) {
        IPlayer* p = player.get();
        if (folded_players_.count(p) > 0) {
            continue;
        }
        const uint32_t bet = round_bets_.at(p);
        if (bet < current_bet_ && all_in_players_.count(p) == 0) {
            return false;
        }
    }
    return true;
}

bool GameSession::can_player_act(IPlayer* player) const
{
    return folded_players_.count(player) == 0 && all_in_players_.count(player) == 0;
}

void GameSession::ask_current_player()
{
    IPlayer* current = players_[current_player_index_].get();
    if (!can_player_act(current)) {
        advance_to_next_player();
        if (is_round_complete()) {
            next_phase();
        } else {
            ask_current_player();
        }
        return;
    }

    poker::protocol::YourTurn turn;
    turn.available_actions = { poker::protocol::Action::Fold };

    const uint32_t bet = round_bets_[current];
    const uint32_t to_call = current_bet_ - bet;

    if (to_call == 0) {
        turn.available_actions.push_back(poker::protocol::Action::Check);
    } else if (to_call <= chips_[current]) {
        turn.available_actions.push_back(poker::protocol::Action::Call);
    } else {
        turn.available_actions.push_back(poker::protocol::Action::Call);
    }

    if (chips_[current] > to_call) {
        turn.available_actions.push_back(poker::protocol::Action::Raise);
    }

    turn.min_amount = current_bet_ + 1;
    turn.max_amount = bet + chips_[current];
    current->send_message(poker::protocol::ServerMessage { turn });
}

void GameSession::advance_to_next_player()
{
    current_player_index_ = find_next_active_player();
}

void GameSession::mark_all_in_if_needed(IPlayer* player)
{
    if (chips_[player] == 0) {
        all_in_players_.insert(player);
        spdlog::info("Player '{}' is all-in", player->get_name());
    }
}

void GameSession::do_showdown()
{
    phase_ = poker::protocol::GamePhase::Showdown;
    broadcast_state();

    std::vector<IPlayer*> contenders;
    for (const auto& player : players_) {
        if (folded_players_.count(player.get()) == 0) {
            contenders.push_back(player.get());
        }
    }

    if (contenders.empty()) {
        start_new_hand();
        return;
    }

    if (contenders.size() == 1) {
        handle_single_winner(contenders.front());
        return;
    }

    poker::HandValue best;
    std::vector<IPlayer*> winners;

    for (IPlayer* player : contenders) {
        std::vector<poker::Card> cards = hands_[player];
        cards.insert(cards.end(), community_cards_.begin(), community_cards_.end());
        const poker::HandValue value = poker::evaluate_best(cards);

        if (winners.empty() || value > best) {
            best = value;
            winners = { player };
        } else if (value == best) {
            winners.push_back(player);
        }
    }

    award_pot(winners);
    start_new_hand();
}

void GameSession::award_pot(const std::vector<IPlayer*>& winners)
{
    if (winners.empty() || pot_ == 0) {
        return;
    }

    const uint32_t share = pot_ / static_cast<uint32_t>(winners.size());
    uint32_t remainder = pot_ % static_cast<uint32_t>(winners.size());

    std::vector<std::string> winner_names;
    winner_names.reserve(winners.size());

    for (IPlayer* winner : winners) {
        chips_[winner] += share;
        winner_names.push_back(winner->get_name());
        spdlog::info("Player '{}' wins {} chips", winner->get_name(), share);
    }

    if (remainder > 0) {
        chips_[winners.front()] += remainder;
    }

    broadcast_hand_result(winner_names, pot_);
    pot_ = 0;
}

void GameSession::broadcast_hand_result(const std::vector<std::string>& winner_names, uint32_t amount)
{
    poker::protocol::HandResult result;
    result.winner_names = winner_names;
    result.pot_amount = amount;

    for (const auto& player : players_) {
        player->send_message(poker::protocol::ServerMessage { result });
    }
}

void GameSession::handle_single_winner(IPlayer* winner)
{
    spdlog::info("Player '{}' wins by fold", winner->get_name());
    award_pot({ winner });
    start_new_hand();
}

void GameSession::start_new_hand()
{
    dealer_index_ = (dealer_index_ + 1) % players_.size();
    start();
}
