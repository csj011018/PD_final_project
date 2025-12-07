#include "Deck.h"
#include <random>
#include <chrono>
#include <stdexcept>
#include <algorithm>

using namespace std;

Deck::Deck() {
    init();
}

void Deck::init() {
    cards.clear();
    for (int r = 3; r <= 15; ++r) {
        cards.emplace_back(Suit::Spade,   r);
        cards.emplace_back(Suit::Heart,   r);
        cards.emplace_back(Suit::Club,    r);
        cards.emplace_back(Suit::Diamond, r);
    }
    cards.emplace_back(Suit::Joker, 16);
    cards.emplace_back(Suit::Joker, 17);
}

void Deck::shuffle() {
    unsigned seed = static_cast<unsigned>(
        chrono::system_clock::now().time_since_epoch().count()
    );
    std::shuffle(cards.begin(), cards.end(), default_random_engine(seed));
}

bool Deck::empty() const {
    return cards.empty();
}

Card Deck::draw() {
    if (cards.empty()) {
        throw runtime_error("Deck is empty, cannot draw.");
    }
    Card c = cards.back();
    cards.pop_back();
    return c;
}

size_t Deck::size() const {
    return cards.size();
}
