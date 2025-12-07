#ifndef CHARACTER_H
#define CHARACTER_H

#include <string>
#include <vector>
#include "Card.h"

// ==============================
// 抽象角色 / 物品 / 撲克牌角色
// ==============================

class Character {
protected:
    std::string name;
    int health;

public:
    Character(const std::string& n, int hp);
    virtual ~Character();

    std::string getName() const;
    int getHealth() const;

    virtual void attack(Character& target) = 0;
};

class Item {
private:
    std::string name;

public:
    Item(const std::string& n);
    std::string getName() const;
};

class CardCharacter : public Character {
protected:
    std::vector<Card> hand;

public:
    CardCharacter(const std::string& n, int hp = 100);
    virtual ~CardCharacter();

    const std::string& getNameRef() const;

    void addCard(const Card& c);
    void sortHand();
    void printHand() const;

    size_t handSize() const;
    bool isHandEmpty() const;
    const Card& getCard(size_t index) const;

    std::vector<Card> playCardsByIndices(const std::vector<int>& indices);
    const std::vector<Card>& getHand() const;

    virtual Move playTurn(const Move& lastMove) = 0;
};

// =======================
// Player / Enemy 宣告
// =======================

class Player : public CardCharacter {
public:
    Player(const std::string& n);

    void attack(Character& target) override;
    Move playTurn(const Move& lastMove) override;
};

class Enemy : public CardCharacter {
public:
    Enemy(const std::string& n);

    void attack(Character& target) override;
    Move playTurn(const Move& lastMove) override;

private:
    bool findSingleGreater(int targetRank, std::vector<int>& outIdx);
    bool findPairGreater(int targetRank, std::vector<int>& outIdx);
    bool findBomb(int targetRank, bool mustGreater, std::vector<int>& outIdx, int& bombRank);
    bool findRocket(std::vector<int>& outIdx);
    bool findStraightGreater(int targetHighRank, int length, std::vector<int>& outIdx);
    bool findFullHouseGreater(int targetTripleRank, std::vector<int>& outIdx);

    bool findAnyPair(std::vector<int>& outIdx);
    bool findAnyStraightLen5(std::vector<int>& outIdx);
    bool findAnyFullHouse(std::vector<int>& outIdx);
    bool findLeadSingle(std::vector<int>& outIdx);
};

#endif // CHARACTER_H
