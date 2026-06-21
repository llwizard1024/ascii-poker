#pragma once

#include "i_player.h"

#include <poker/card.h>
#include <poker/deck.h>
#include <poker/protocol.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class GameSession {
public:
    GameSession(std::vector<std::shared_ptr<IPlayer>> players, uint64_t room_id);
    void start();
    void next_phase();
    void apply_action(std::shared_ptr<IPlayer> player, poker::protocol::Action action, std::optional<uint32_t> amount);
    void broadcast_state();

private:
    std::vector<std::shared_ptr<IPlayer>> players_;
    uint64_t room_id_;
    poker::Deck deck_;
    std::vector<poker::Card> community_cards_;
    std::unordered_map<IPlayer*, std::vector<poker::Card>> hands_;
    poker::protocol::GamePhase phase_;
};