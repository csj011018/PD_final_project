#include "Card.h"
#include <algorithm>
#include <ostream>
#include <string>
#include <vector>

using namespace std;

// ======================
// Suit / Card
// ======================

string suitToString(Suit s) {
    switch (s) {
    case Suit::Spade:   return "S";      // Spade
    case Suit::Heart:   return "H";      // Heart
    case Suit::Club:    return "C";      // Club
    case Suit::Diamond: return "D";      // Diamond
    case Suit::Joker:   return "Joker";
    default:            return "?";
    }
}

Card::Card(Suit s, int r)
    : suit(s), rank(r) {}

string Card::toString() const {
    // Jokers
    if (suit == Suit::Joker) {
        if (rank == 16) return "Joker(S)"; // small joker
        if (rank == 17) return "Joker(B)"; // big joker
        return "Joker(?)";
    }

    // Normal ranks
    string rankStr;
    if (rank >= 3 && rank <= 10) {
        rankStr = to_string(rank);
    } else if (rank == 11) {
        rankStr = "J";
    } else if (rank == 12) {
        rankStr = "Q";
    } else if (rank == 13) {
        rankStr = "K";
    } else if (rank == 14) {
        rankStr = "A";
    } else if (rank == 15) {
        rankStr = "2";
    } else {
        rankStr = "?";
    }

    // Example: "S3", "HK", "D10", "C2"
    return suitToString(suit) + rankStr;
}

bool Card::operator<(const Card& other) const {
    return rank < other.rank;
}

ostream& operator<<(ostream& os, const Card& c) {
    os << c.toString();
    return os;
}

// ======================
// HandType / Move
// ======================

string handTypeToString(HandType t) {
    switch (t) {
    case HandType::Pass:      return "Pass";
    case HandType::Single:    return "Single";
    case HandType::Pair:      return "Pair";
    case HandType::Straight:  return "Straight";
    case HandType::FullHouse: return "Full House";
    case HandType::Bomb:      return "Bomb";
    case HandType::Rocket:    return "Rocket";
    case HandType::Invalid:
    default:                  return "Invalid";
    }
}

bool isStraight(vector<Card> cards) {
    sort(cards.begin(), cards.end(), [](const Card& a, const Card& b) {
        return a.rank < b.rank;
    });

    // Disallow 2 and Jokers in a straight
    for (const auto& c : cards) {
        if (c.rank < 3 || c.rank > 14) {
            return false;
        }
    }

    for (size_t i = 0; i + 1 < cards.size(); ++i) {
        if (cards[i + 1].rank != cards[i].rank + 1) {
            return false;
        }
    }
    return true;
}

pair<HandType, int> analyzeHand(const vector<Card>& cards) {
    if (cards.empty()) {
        return { HandType::Pass, -1 };
    }

    vector<Card> v = cards;
    sort(v.begin(), v.end(), [](const Card& a, const Card& b) {
        return a.rank < b.rank;
    });

    size_t n = v.size();

    // Single
    if (n == 1) {
        return { HandType::Single, v[0].rank };
    }

    // Two cards: pair or rocket
    if (n == 2) {
        if (v[0].suit == Suit::Joker && v[1].suit == Suit::Joker) {
            return { HandType::Rocket, 100 };
        }
        if (v[0].rank == v[1].rank) {
            return { HandType::Pair, v[0].rank };
        }
        return { HandType::Invalid, -1 };
    }

    // Four cards: bomb
    if (n == 4) {
        bool allSame = (v[0].rank == v[1].rank &&
                        v[1].rank == v[2].rank &&
                        v[2].rank == v[3].rank);
        if (allSame) {
            return { HandType::Bomb, v[0].rank };
        }
        return { HandType::Invalid, -1 };
    }

    // Five cards: straight or full house
    if (n == 5) {
        if (isStraight(v)) {
            return { HandType::Straight, v.back().rank };
        }

        bool case1 = (v[0].rank == v[1].rank &&
                      v[1].rank == v[2].rank &&
                      v[3].rank == v[4].rank &&
                      v[2].rank != v[3].rank);

        bool case2 = (v[0].rank == v[1].rank &&
                      v[2].rank == v[3].rank &&
                      v[3].rank == v[4].rank &&
                      v[1].rank != v[2].rank);

        if (case1) {
            return { HandType::FullHouse, v[2].rank };
        }
        if (case2) {
            return { HandType::FullHouse, v[4].rank };
        }
        return { HandType::Invalid, -1 };
    }

    return { HandType::Invalid, -1 };
}

Move::Move()
    : type(HandType::Pass), cards(), mainRank(-1) {}

Move::Move(HandType t, const vector<Card>& cs, int mr)
    : type(t), cards(cs), mainRank(mr) {}

bool Move::isPass() const {
    return type == HandType::Pass;
}

// ======================
// canBeat
// ======================

bool canBeat(const Move& prev, const Move& now) {
    if (now.type == HandType::Invalid || now.type == HandType::Pass) {
        return false;
    }

    if (prev.type == HandType::Pass) {
        return true;
    }

    if (prev.type == HandType::Rocket) {
        return false;
    }
    if (now.type == HandType::Rocket) {
        return true;
    }

    if (now.type == HandType::Bomb && prev.type != HandType::Bomb) {
        return true;
    }

    if (now.type == HandType::Bomb && prev.type == HandType::Bomb) {
        return now.mainRank > prev.mainRank;
    }

    if (now.type != prev.type) {
        return false;
    }
    if (now.cards.size() != prev.cards.size()) {
        return false;
    }

    return now.mainRank > prev.mainRank;
}
