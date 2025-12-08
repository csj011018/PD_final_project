#ifndef CHARACTER_H
#define CHARACTER_H

#include <string>
#include <vector>
#include "Card.h"

class CardCharacter {
protected:
    std::string name;
    std::vector<Card> hand;

public:
    CardCharacter(const std::string& n);
    virtual ~CardCharacter() = default;

    const std::string& getNameRef() const { return name; }

    void addCard(const Card& c);
    void sortHand();
    void printHand() const;

    std::size_t handSize() const { return hand.size(); }
    bool isHandEmpty() const { return hand.empty(); }

    const std::vector<Card>& getHand() const { return hand; }
    const Card& getCard(std::size_t index) const;

    std::vector<Card> playCardsByIndices(const std::vector<int>& indices);

    virtual Move playTurn(const Move& lastMove) = 0;
};

// Human player
class Player : public CardCharacter {
public:
    using CardCharacter::CardCharacter;

    // Console version (interactive)
    Move playTurn(const Move& lastMove) override;

    // GUI helper: use given indices, do not read from stdin
    Move playTurnWithIndices(const Move& lastMove,
                             const std::vector<int>& indices,
                             bool& ok,
                             std::string& message);
};

// Simple AI
class Enemy : public Player {
public:
    Enemy(const std::string& n) : Player(n), bombDecisionProb(1.0) {}

    Move playTurn(const Move& lastMove) override;

    // Game（例如 main_sfml）會根據「上一手出牌玩家的剩餘張數」
    // 來設定這個機率：0.0 ~ 1.0
    void setBombDecisionProb(double p) {
        if (p < 0.0) p = 0.0;
        if (p > 1.0) p = 1.0;
        bombDecisionProb = p;
    }

private:
    // 主要 AI 會用到的 helper
    bool findSingleGreater(int targetRank, std::vector<int>& outIdx) const;
    bool findPairGreater(int targetRank, std::vector<int>& outIdx) const;
    bool findBomb(int targetRank, bool mustGreater, std::vector<int>& outIdx) const;
    bool findRocket(std::vector<int>& outIdx) const;
    bool findStraightGreater(int targetHighRank, int length, std::vector<int>& outIdx) const;
    bool findFullHouseGreater(int targetTripleRank, std::vector<int>& outIdx) const;

    // 開新一輪（桌上是 Pass）時專用：
    // 先找「任意 5 張順子」、再找「任意葫蘆」、再找「低點數對子」。
    bool findAnyStraight(int length, std::vector<int>& outIdx) const;
    bool findAnyFullHouse(std::vector<int>& outIdx) const;
    bool findOpeningPair(std::vector<int>& outIdx) const; // 避免 AA / 22 / JokerJoker

    // 炸彈 / 火箭 出不出的機率
    double bombDecisionProb;
};

#endif // CHARACTER_H
