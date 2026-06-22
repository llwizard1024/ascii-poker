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
    GameSession(std::vector<std::shared_ptr<IPlayer>> players, uint64_t room_id);
    void start();
    void next_phase();
    void apply_action(std::shared_ptr<Session> session, poker::protocol::Action action, std::optional<uint32_t> amount);
    void broadcast_state();
    IPlayer* get_player(Session* session) const;
    std::unordered_map<Session*, IPlayer*> session_map_;

private:
    size_t find_next_active_player() const;
    bool is_round_complete() const;
    void ask_current_player();
    void advance_to_next_player();

    std::vector<std::shared_ptr<IPlayer>> players_;
    std::unordered_map<IPlayer*, std::vector<poker::Card>> hands_;
    std::unordered_set<IPlayer*> folded_players_;

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