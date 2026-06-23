#pragma once

#include "../network/session.h"
#include "i_player.h"

#include <poker/card.h>
#include <poker/deck.h>
#include <poker/protocol.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class GameSession {
public:
    GameSession(
        std::vector<std::shared_ptr<IPlayer>> players,
        std::unordered_map<Session*, IPlayer*> session_map,
        uint64_t room_id);

    void start();
    void apply_action(std::shared_ptr<Session> session, poker::protocol::Action action, std::optional<uint32_t> amount);
    void on_player_disconnect(std::shared_ptr<Session> session);
    void broadcast_state();

    IPlayer* get_player(Session* session) const;

private:
    size_t count_active_players() const;
    size_t find_next_active_player() const;
    bool is_round_complete() const;
    bool can_player_act(IPlayer* player) const;
    void ask_current_player();
    void advance_to_next_player();
    void mark_all_in_if_needed(IPlayer* player);
    void next_phase();
    void do_showdown();
    void award_pot(const std::vector<IPlayer*>& winners);
    void broadcast_hand_result(const std::vector<std::string>& winner_names, uint32_t amount);
    void handle_single_winner(IPlayer* winner);
    void start_new_hand();

    std::vector<std::shared_ptr<IPlayer>> players_;
    std::unordered_map<Session*, IPlayer*> session_map_;
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

    std::unordered_map<IPlayer*, uint32_t> chips_;
    std::unordered_map<IPlayer*, uint32_t> round_bets_;
};
