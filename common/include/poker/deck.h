#pragma once

#include "poker/card.h"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <vector>

namespace poker {
class Deck {
private:
    std::vector<Card> m_cards;

public:
    Deck();
    void shuffle();

    std::vector<Card> deal(size_t count);

    bool empty() const noexcept
    {
        return m_cards.empty();
    }

    size_t size() const noexcept
    {
        return m_cards.size();
    }
};
}