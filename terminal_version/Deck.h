#ifndef DECK_H
#define DECK_H

#include <vector>
#include "Card.h"

class Deck {
private:
    std::vector<Card> cards;

public:
    Deck();

    void init();
    void shuffle();
    bool empty() const;
    Card draw();
    size_t size() const;
};

#endif // DECK_H
