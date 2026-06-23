#include "game/game_session.h"

#include <poker/error_codes.h>
#include <poker/game_constants.h>
#include <poker/hand_evaluator.h>
#include <poker/side_pots.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <iterator>

namespace poker::server {

GameSession::GameSession(
    std::vector<std::shared_ptr<IPlayer>> players,
    std::unordered_map<IConnection*, IPlayer*> connection_map,
    uint64_t room_id)
    : players_(std::move(players))
    , connection_map_(std::move(connection_map))
    , room_id_(room_id)
    , phase_(poker::protocol::GamePhase::PreFlop)
{
    spdlog::info("GameSession created for room {} with {} players", room_id_, players_.size());
}

size_t GameSession::sb_index() const
{
    if (players_.size() == 2) {
        return dealer_index_;
    }
    return (dealer_index_ + 1) % players_.size();
}

size_t GameSession::bb_index() const
{
    if (players_.size() == 2) {
        return (dealer_index_ + 1) % players_.size();
    }
    return (dealer_index_ + 2) % players_.size();
}

size_t GameSession::first_to_act_preflop() const
{
    if (players_.size() == 2) {
        return dealer_index_;
    }
    return (bb_index() + 1) % players_.size();
}

size_t GameSession::first_to_act_postflop() const
{
    size_t idx = (dealer_index_ + 1) % players_.size();
    const size_t start = idx;

    do {
        if (can_player_act(players_[idx].get())) {
            return idx;
        }
        idx = (idx + 1) % players_.size();
    } while (idx != start);

    return dealer_index_;
}

uint32_t GameSession::contribute(IPlayer* player, uint32_t amount)
{
    const uint32_t actual = std::min(amount, chips_[player]);
    chips_[player] -= actual;
    round_bets_[player] += actual;
    total_contributed_[player] += actual;
    pot_ += actual;
    mark_all_in_if_needed(player);
    return actual;
}

void GameSession::post_blinds()
{
    IPlayer* small_blind = players_[sb_index()].get();
    IPlayer* big_blind = players_[bb_index()].get();

    contribute(small_blind, poker::SMALL_BLIND);
    contribute(big_blind, poker::BIG_BLIND);

    current_bet_ = round_bets_[big_blind];

    spdlog::info("Blinds posted: SB '{}' ({}), BB '{}' ({})",
        small_blind->get_name(),
        round_bets_[small_blind],
        big_blind->get_name(),
        round_bets_[big_blind]);
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
    last_raise_increment_ = poker::BIG_BLIND;
    total_contributed_.clear();

    for (const auto& player : players_) {
        if (chips_.find(player.get()) == chips_.end()) {
            chips_[player.get()] = poker::STARTING_CHIPS;
        }
        auto hand = deck_.deal(2);
        hands_[player.get()] = std::move(hand);
        round_bets_[player.get()] = 0;
        total_contributed_[player.get()] = 0;
        spdlog::info("Player '{}' received hand: {}, {}",
            player->get_name(),
            hands_[player.get()][0].to_string(),
            hands_[player.get()][1].to_string());
    }

    post_blinds();
    current_player_index_ = first_to_act_preflop();
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
    last_raise_increment_ = poker::BIG_BLIND;
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

    current_player_index_ = first_to_act_postflop();
    broadcast_state();
    ask_current_player();
}

void GameSession::remove_player(ConnectionPtr connection)
{
    IPlayer* player = get_player(connection.get());
    if (!player) {
        return;
    }

    const auto player_it = std::find_if(players_.begin(), players_.end(),
        [player](const std::shared_ptr<IPlayer>& p) { return p.get() == player; });
    if (player_it == players_.end()) {
        return;
    }

    const size_t removed_index = static_cast<size_t>(std::distance(players_.begin(), player_it));
    const bool was_current = players_[current_player_index_].get() == player;
    const bool already_folded = folded_players_.count(player) > 0;

    if (!already_folded) {
        folded_players_.insert(player);
        spdlog::info("Player '{}' left – auto-fold", player->get_name());
    }

    if (count_active_players() == 1) {
        award_all_pots();
    }

    erase_player_state(player, connection);
    players_.erase(player_it);

    if (players_.empty()) {
        return;
    }

    fix_indices_after_removal(removed_index);

    if (!has_enough_players()) {
        return;
    }

    if (count_active_players() == 1) {
        try_start_new_hand();
        return;
    }

    continue_after_player_left(was_current);
}

void GameSession::erase_player_state(IPlayer* player, ConnectionPtr connection)
{
    hands_.erase(player);
    chips_.erase(player);
    round_bets_.erase(player);
    total_contributed_.erase(player);
    folded_players_.erase(player);
    all_in_players_.erase(player);
    connection_map_.erase(connection.get());
}

void GameSession::fix_indices_after_removal(size_t removed_index)
{
    if (dealer_index_ >= players_.size()) {
        dealer_index_ = 0;
    } else if (removed_index < dealer_index_) {
        --dealer_index_;
    }

    if (current_player_index_ >= players_.size()) {
        current_player_index_ = 0;
    } else if (removed_index < current_player_index_) {
        --current_player_index_;
    }
}

void GameSession::continue_after_player_left(bool was_current)
{
    if (was_current || is_round_complete()) {
        if (is_round_complete()) {
            next_phase();
        } else {
            ask_current_player();
        }
    } else {
        broadcast_state();
    }
}

void GameSession::apply_action(ConnectionPtr connection, poker::protocol::Action action, std::optional<uint32_t> amount)
{
    IPlayer* current = get_player(connection.get());
    if (!current) {
        spdlog::error("Connection not mapped to any player");
        return;
    }

    if (players_[current_player_index_].get() != current) {
        current->send_message(poker::protocol::ServerMessage {
            poker::protocol::make_error(poker::protocol::ErrorCode::NotYourTurn) });
        return;
    }

    const uint32_t bet = round_bets_[current];

    switch (action) {
    case poker::protocol::Action::Fold:
        folded_players_.insert(current);
        spdlog::info("Player '{}' folds", current->get_name());
        break;
    case poker::protocol::Action::Check:
        if (current_bet_ != bet) {
            current->send_message(poker::protocol::ServerMessage {
                poker::protocol::make_error(poker::protocol::ErrorCode::CannotCheck) });
            return;
        }
        break;
    case poker::protocol::Action::Call: {
        const uint32_t to_call = current_bet_ - bet;
        if (to_call > 0) {
            contribute(current, to_call);
        }
    } break;
    case poker::protocol::Action::Raise: {
        const uint32_t min_raise_total = current_bet_ + last_raise_increment_;
        const uint32_t all_in_total = bet + chips_[current];

        if (!amount || *amount <= current_bet_) {
            current->send_message(poker::protocol::ServerMessage {
                poker::protocol::make_error(poker::protocol::ErrorCode::InvalidRaise) });
            return;
        }
        if (*amount <= bet) {
            current->send_message(poker::protocol::ServerMessage {
                poker::protocol::make_error(poker::protocol::ErrorCode::InvalidRaise) });
            return;
        }
        if (*amount < min_raise_total && *amount < all_in_total) {
            current->send_message(poker::protocol::ServerMessage {
                poker::protocol::make_error(poker::protocol::ErrorCode::InvalidRaise) });
            return;
        }
        const uint32_t to_put = *amount - bet;
        if (to_put > chips_[current]) {
            current->send_message(poker::protocol::ServerMessage {
                poker::protocol::make_error(poker::protocol::ErrorCode::NotEnoughChips) });
            return;
        }
        const uint32_t previous_bet = current_bet_;
        contribute(current, to_put);
        if (round_bets_[current] > current_bet_) {
            last_raise_increment_ = round_bets_[current] - previous_bet;
            current_bet_ = round_bets_[current];
        }
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

IPlayer* GameSession::get_player(IConnection* connection) const
{
    const auto it = connection_map_.find(connection);
    return it != connection_map_.end() ? it->second : nullptr;
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
        const uint32_t player_bet = round_bets_.at(p);
        if (player_bet < current_bet_ && all_in_players_.count(p) == 0) {
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
    const uint32_t min_raise_total = current_bet_ + last_raise_increment_;
    const uint32_t all_in_total = bet + chips_[current];

    if (to_call == 0) {
        turn.available_actions.push_back(poker::protocol::Action::Check);
    } else if (chips_[current] > 0) {
        turn.available_actions.push_back(poker::protocol::Action::Call);
    }

    if (chips_[current] > to_call && all_in_total >= min_raise_total) {
        turn.available_actions.push_back(poker::protocol::Action::Raise);
    }

    turn.min_amount = min_raise_total;
    turn.max_amount = all_in_total;
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

std::vector<IPlayer*> GameSession::find_winners_among(const std::vector<IPlayer*>& contenders) const
{
    if (contenders.empty()) {
        return {};
    }

    if (contenders.size() == 1) {
        return { contenders.front() };
    }

    poker::HandValue best;
    std::vector<IPlayer*> winners;

    for (IPlayer* player : contenders) {
        std::vector<poker::Card> cards = hands_.at(player);
        cards.insert(cards.end(), community_cards_.begin(), community_cards_.end());
        const poker::HandValue value = poker::evaluate_best(cards);

        if (winners.empty() || value > best) {
            best = value;
            winners = { player };
        } else if (value == best) {
            winners.push_back(player);
        }
    }

    return winners;
}

void GameSession::award_all_pots()
{
    if (pot_ == 0) {
        return;
    }

    std::vector<poker::PotPlayerState> states(players_.size());
    for (size_t i = 0; i < players_.size(); ++i) {
        IPlayer* player = players_[i].get();
        states[i].contributed = total_contributed_[player];
        states[i].folded = folded_players_.count(player) > 0;
    }

    const auto side_pots = poker::compute_side_pots(states);

    std::vector<std::string> winner_names;
    uint32_t total_awarded = 0;

    for (const auto& side_pot : side_pots) {
        std::vector<IPlayer*> contenders;
        contenders.reserve(side_pot.eligible_player_indices.size());
        for (const size_t index : side_pot.eligible_player_indices) {
            contenders.push_back(players_[index].get());
        }

        const auto winners = find_winners_among(contenders);
        if (winners.empty() || side_pot.amount == 0) {
            continue;
        }

        const uint32_t share = side_pot.amount / static_cast<uint32_t>(winners.size());
        const uint32_t remainder = side_pot.amount % static_cast<uint32_t>(winners.size());

        for (IPlayer* winner : winners) {
            chips_[winner] += share;
            const auto& name = winner->get_name();
            if (std::find(winner_names.begin(), winner_names.end(), name) == winner_names.end()) {
                winner_names.push_back(name);
            }
            spdlog::info("Player '{}' wins {} chips from a {}-chip pot",
                name,
                share,
                side_pot.amount);
        }

        if (remainder > 0) {
            chips_[winners.front()] += remainder;
        }

        total_awarded += side_pot.amount;
    }

    if (!winner_names.empty()) {
        broadcast_hand_result(winner_names, total_awarded);
    }

    pot_ = 0;
    total_contributed_.clear();
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
        try_start_new_hand();
        return;
    }

    if (contenders.size() == 1) {
        handle_single_winner(contenders.front());
        return;
    }

    award_all_pots();
    try_start_new_hand();
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
    award_all_pots();
    try_start_new_hand();
}

void GameSession::try_start_new_hand()
{
    if (has_enough_players()) {
        start_new_hand();
    }
}

void GameSession::start_new_hand()
{
    dealer_index_ = (dealer_index_ + 1) % players_.size();
    start();
}

} // namespace poker::server
