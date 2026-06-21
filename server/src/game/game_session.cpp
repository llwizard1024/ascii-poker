#include "game_session.h"

GameSession::GameSession(std::vector<std::shared_ptr<IPlayer>> players, uint64_t room_id)
    : players_(players)
    , room_id_(room_id)
    , phase_(poker::protocol::GamePhase::PreFlop)
{
}
