#include "game_session.h"

#include <spdlog/spdlog.h>

GameSession::GameSession(std::vector<std::shared_ptr<IPlayer>> players, uint64_t room_id)
    : players_(std::move(players))
    , room_id_(room_id)
    , phase_(poker::protocol::GamePhase::PreFlop)
{
    spdlog::info("GameSession created for room {} with {} players", room_id_, players_.size());
}

void GameSession::start()
{
    deck_.shuffle();

    for (const auto& player : players_) {
        auto hand = deck_.deal(2);
        hands_[player.get()] = std::move(hand);
        spdlog::info("Player '{}' received hand: {}, {}",
            player->get_name(),
            hands_[player.get()][0].to_string(),
            hands_[player.get()][1].to_string());
    }

    phase_ = poker::protocol::GamePhase::PreFlop;
    broadcast_state();
}

void GameSession::next_phase()
{
    switch (phase_) {
    case poker::protocol::GamePhase::PreFlop: {
        auto flop = deck_.deal(3);
        community_cards_.insert(community_cards_.end(), flop.begin(), flop.end());
        phase_ = poker::protocol::GamePhase::Flop;
        spdlog::info("Flop: {}, {}, {}",
            flop[0].to_string(), flop[1].to_string(), flop[2].to_string());
    } break;
    case poker::protocol::GamePhase::Flop: {
        auto turn = deck_.deal(1);
        community_cards_.push_back(turn[0]);
        phase_ = poker::protocol::GamePhase::Turn;
        spdlog::info("Turn: {}", turn[0].to_string());
    } break;
    case poker::protocol::GamePhase::Turn: {
        auto river = deck_.deal(1);
        community_cards_.push_back(river[0]);
        phase_ = poker::protocol::GamePhase::River;
        spdlog::info("River: {}", river[0].to_string());
    } break;
    case poker::protocol::GamePhase::River:
        phase_ = poker::protocol::GamePhase::Showdown;
        spdlog::info("Showdown!");
        break;
    case poker::protocol::GamePhase::Showdown:
        spdlog::warn("next_phase called during Showdown – ignoring");
        return;
    }

    broadcast_state();
}

void GameSession::broadcast_state()
{
    for (const auto& player : players_) {
        poker::protocol::GameStateUpdate update;
        update.general_cards = community_cards_;
        update.your_cards = hands_[player.get()];
        update.phase = phase_;
        update.active_player_name = "";
        update.total_pot = 0;

        player->send_message(poker::protocol::ServerMessage { update });
    }
}

void GameSession::apply_action(std::shared_ptr<IPlayer> player, poker::protocol::Action action, std::optional<uint32_t> amount)
{
    const std::string action_str = [&]() {
        switch (action) {
        case poker::protocol::Action::Fold:
            return "fold";
        case poker::protocol::Action::Check:
            return "check";
        case poker::protocol::Action::Call:
            return "call";
        case poker::protocol::Action::Raise:
            return "raise";
        default:
            return "unknown";
        }
    }();

    spdlog::info("Player '{}' action: {} (amount: {})",
        player->get_name(),
        action_str,
        amount.has_value() ? std::to_string(*amount) : "none");
}