#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

namespace poker {
enum class Suit : uint8_t {
    Clubs = 0,
    Diamonds = 1,
    Hearts = 2,
    Spades = 3
};

enum class Rank : uint8_t {
    Two = 0,
    Three = 1,
    Four = 2,
    Five = 3,
    Six = 4,
    Seven = 5,
    Eight = 6,
    Nine = 7,
    Ten = 8,
    Jack = 9,
    Queen = 10,
    King = 11,
    Ace = 12
};

inline constexpr std::array<std::string_view, 4> SUIT_SYMBOLS = { "C", "D", "H", "S" };
inline constexpr std::array<std::string_view, 13> RANK_SYMBOLS = { "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A" };

inline std::string_view get_suit_symbol(Suit suit) { return SUIT_SYMBOLS[static_cast<int>(suit)]; }
inline std::string_view get_rank_symbol(Rank rank) { return RANK_SYMBOLS[static_cast<int>(rank)]; }

class Card {
private:
    Suit m_suit;
    Rank m_rank;

public:
    Card(Suit suit, Rank rank)
        : m_suit(suit)
        , m_rank(rank)
    {
    }

    Suit suit() const noexcept { return m_suit; }
    Rank rank() const noexcept { return m_rank; }

    bool operator==(const Card& other) const noexcept
    {
        return m_rank == other.m_rank && m_suit == other.m_suit;
    }

    bool operator<(const Card& other) const noexcept
    {
        if (m_rank != other.m_rank) {
            return m_rank < other.m_rank;
        }
        return m_suit < other.m_suit;
    }

    bool operator>(const Card& other) const noexcept
    {
        return other < *this;
    }

    std::string to_string() const
    {
        return std::string(get_rank_symbol(m_rank)) + std::string(get_suit_symbol(m_suit));
    }
};

inline std::ostream& operator<<(std::ostream& os, const Card& card)
{
    return os << card.to_string();
}
}