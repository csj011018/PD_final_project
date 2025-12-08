#include "Deck.h"
#include <random>
#include <chrono>
#include <stdexcept>

Deck::Deck() {
    init();
}

void Deck::init() {
    cards.clear();

    // Normal cards 3–2 (rank 3..15)
    for (int r = 3; r <= 15; ++r) {
        cards.emplace_back(Suit::Spade,   r);
        cards.emplace_back(Suit::Heart,   r);
        cards.emplace_back(Suit::Club,    r);
        cards.emplace_back(Suit::Diamond, r);
    }

    // Jokers
    cards.emplace_back(Suit::Joker, 16); // small
    cards.emplace_back(Suit::Joker, 17); // big
}

void Deck::shuffle() {
    unsigned seed = static_cast<unsigned>(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    std::shuffle(cards.begin(), cards.end(), std::default_random_engine(seed));
}

bool Deck::empty() const {
    return cards.empty();
}

Card Deck::draw() {
    if (cards.empty()) {
        throw std::runtime_error("Deck is empty");
    }
    Card c = cards.back();
    cards.pop_back();
    return c;
}

std::size_t Deck::size() const {
    return cards.size();
}
