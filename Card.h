#ifndef CARD_H
#define CARD_H

#include <string>
#include <vector>
#include <utility>
#include <ostream>

// =========================
// 牌、牌型、出牌 Move 宣告
// =========================

enum class Suit {
    Spade,   // 黑桃
    Heart,   // 紅心
    Club,    // 梅花
    Diamond, // 方塊
    Joker    // 王（大小王）
};

std::string suitToString(Suit s);

struct Card {
    Suit suit;
    int  rank;

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
    Bomb,   // 四張同點數
    Rocket  // 兩張 Joker（火箭）
};

std::string handTypeToString(HandType t);

// 判斷是否為順子（只負責連不連與合法，長度由外面控制）
bool isStraight(std::vector<Card> cards);

// 分析一組牌：回傳 (牌型, 主點數)
std::pair<HandType, int> analyzeHand(const std::vector<Card>& cards);

struct Move {
    HandType type;
    std::vector<Card> cards;
    int mainRank;

    Move();
    Move(HandType t, const std::vector<Card>& cs, int mr);

    bool isPass() const;
};

// 兩次出牌是否可以被後者壓過
bool canBeat(const Move& prev, const Move& now);

#endif // CARD_H
