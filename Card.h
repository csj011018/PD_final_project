#ifndef CARD_H
#define CARD_H

#include <string>
#include <vector>
#include <utility>
#include <iosfwd>

enum class Suit {
    Spade,
    Heart,
    Club,
    Diamond,
    Joker
};

std::string suitToString(Suit s);

struct Card {
    Suit suit;
    int  rank; // 3–15 normal, 16 small joker, 17 big joker

    Card(Suit s = Suit::Spade, int r = 3);

    std::string toString() const;
    bool operator<(const Card& other) const;
};

std::ostream& operator<<(std::ostream& os, const Card& c);

enum class HandType {
    Invalid,
    Pass,
    Single,
    Pair,
    Straight,
    FullHouse,
    Bomb,
    Rocket
};

std::string handTypeToString(HandType t);

bool isStraight(std::vector<Card> cards);

// returns (hand type, main rank)
std::pair<HandType, int> analyzeHand(const std::vector<Card>& cards);

struct Move {
    HandType type;
    std::vector<Card> cards;
    int mainRank;

    Move();
    Move(HandType t, const std::vector<Card>& cs, int mr);
    bool isPass() const;
};

bool canBeat(const Move& prev, const Move& now);

#endif // CARD_H
