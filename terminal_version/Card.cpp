#include "Card.h"
#include <algorithm>

using namespace std;

// ======================
// Suit / Card 實作
// ======================

string suitToString(Suit s) {
    switch (s) {
    case Suit::Spade:   return "♠";
    case Suit::Heart:   return "♥";
    case Suit::Club:    return "♣";
    case Suit::Diamond: return "♦";
    case Suit::Joker:   return "Joker";
    default:            return "?";
    }
}

Card::Card(Suit s, int r)
    : suit(s), rank(r) {}

string Card::toString() const {
    if (suit == Suit::Joker) {
        if (rank == 16) return "Joker(小)";
        if (rank == 17) return "Joker(大)";
        return "Joker(?)";
    }

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
// HandType / Move 實作
// ======================

string handTypeToString(HandType t) {
    switch (t) {
    case HandType::Pass:      return "Pass";
    case HandType::Single:    return "單張";
    case HandType::Pair:      return "對子";
    case HandType::Straight:  return "順子";
    case HandType::FullHouse: return "葫蘆";
    case HandType::Bomb:      return "炸彈";
    case HandType::Rocket:    return "火箭";
    case HandType::Invalid:
    default:                  return "非法牌型";
    }
}

bool isStraight(vector<Card> cards) {
    sort(cards.begin(), cards.end(), [](const Card& a, const Card& b) {
        return a.rank < b.rank;
    });

    // 不含 2 與 Joker
    for (const auto& c : cards) {
        if (c.rank < 3 || c.rank > 14) {
            return false;
        }
    }

    // 連號檢查
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

    // 單張
    if (n == 1) {
        return { HandType::Single, v[0].rank };
    }

    // 兩張：可能是火箭或對子
    if (n == 2) {
        if (v[0].suit == Suit::Joker && v[1].suit == Suit::Joker) {
            return { HandType::Rocket, 100 };
        }
        if (v[0].rank == v[1].rank) {
            return { HandType::Pair, v[0].rank };
        }
        return { HandType::Invalid, -1 };
    }

    // 四張：炸彈
    if (n == 4) {
        bool allSame = (v[0].rank == v[1].rank &&
                        v[1].rank == v[2].rank &&
                        v[2].rank == v[3].rank);
        if (allSame) {
            return { HandType::Bomb, v[0].rank };
        }
        return { HandType::Invalid, -1 };
    }

    // 五張：順子或葫蘆
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
// 出牌比較 canBeat
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
