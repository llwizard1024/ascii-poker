#pragma once

#include "game/i_player.h"
#include "network/i_connection.h"

#include <poker/card.h>
#include <poker/deck.h>
#include <poker/protocol.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace poker::server {

class GameSession {
public:
    GameSession(
        std::vector<std::shared_ptr<IPlayer>> players,
        std::unordered_map<IConnection*, IPlayer*> connection_map,
        uint64_t room_id);

    void start();
    void apply_action(ConnectionPtr connection, poker::protocol::Action action, std::optional<uint32_t> amount);
    void remove_player(ConnectionPtr connection);
    void broadcast_state();
    void broadcast_action_event(const std::string& player_name, poker::protocol::Action action, std::optional<uint32_t> amount);

    IPlayer* get_player(IConnection* connection) const;
    size_t player_count() const { return players_.size(); }
    bool has_enough_players() const { return players_.size() >= 2; }
    size_t count_players_with_chips() const;

    uint32_t pot_total() const { return pot_; }

private:
    size_t sb_index() const;
    size_t bb_index() const;
    size_t first_to_act_preflop() const;
    size_t first_to_act_postflop() const;

    size_t count_active_players() const;
    size_t find_next_active_player() const;
    bool is_round_complete() const;
    bool can_player_act(IPlayer* player) const;
    void ask_current_player();
    void advance_to_next_player();
    void mark_all_in_if_needed(IPlayer* player);
    uint32_t contribute(IPlayer* player, uint32_t amount);
    void post_blinds();
    void next_phase();
    void do_showdown();
    void award_all_pots();
    std::vector<IPlayer*> find_winners_among(const std::vector<IPlayer*>& contenders) const;
    void broadcast_hand_result(const std::vector<std::string>& winner_names, uint32_t amount);
    void handle_single_winner(IPlayer* winner);
    void try_start_new_hand();
    void start_new_hand();
    void erase_player_state(IPlayer* player, ConnectionPtr connection);
    void fix_indices_after_removal(size_t removed_index);
    void continue_after_player_left(bool was_current);
    std::vector<poker::protocol::PlayerState> build_player_states(bool reveal_hole_cards) const;

    std::vector<std::shared_ptr<IPlayer>> players_;
    std::unordered_map<IConnection*, IPlayer*> connection_map_;
    std::unordered_map<IPlayer*, std::vector<poker::Card>> hands_;
    std::unordered_set<IPlayer*> folded_players_;
    std::unordered_set<IPlayer*> all_in_players_;

    uint64_t room_id_;
    poker::Deck deck_;
    std::vector<poker::Card> community_cards_;
    poker::protocol::GamePhase phase_;

    size_t dealer_index_ = 0;
    size_t current_player_index_ = 0;
    uint32_t pot_ = 0;
    uint32_t current_bet_ = 0;
    uint32_t last_raise_increment_ = 0;

    std::unordered_map<IPlayer*, uint32_t> chips_;
    std::unordered_map<IPlayer*, uint32_t> round_bets_;
    std::unordered_map<IPlayer*, uint32_t> total_contributed_;
};

} // namespace poker::server
