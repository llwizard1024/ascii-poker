#include "poker/deck.h"

namespace poker {
Deck::Deck()
{
    m_cards.reserve(52);

    for (uint8_t s = 0; s <= 3; ++s) {
        for (uint8_t r = 0; r <= 12; ++r) {
            m_cards.emplace_back(static_cast<Suit>(s), static_cast<Rank>(r));
        }
    }
}

void Deck::shuffle()
{
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(m_cards.begin(), m_cards.end(), g);
}

std::vector<Card> Deck::deal(size_t count)
{
    if (count > m_cards.size()) {
        throw std::out_of_range("В колоде недостаточно карт для раздачи");
    }

    std::vector<Card> dealt_cards;
    dealt_cards.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        dealt_cards.push_back(m_cards.back());
        m_cards.pop_back();
    }

    return dealt_cards;
}
}