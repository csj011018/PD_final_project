#ifndef GAME_H
#define GAME_H

#include <vector>
#include <fstream>
#include "Deck.h"
#include "Character.h"

class Game {
private:
    Deck deck;

    Player player;
    std::vector<Enemy> enemies;
    std::vector<Item> items;

    std::vector<CardCharacter*> playersAll;

    std::ofstream logFile;

    int landlordIndex;
    int currentPlayerIndex;

    Move lastMove;
    int lastMovePlayerIndex;
    int passCountInRound;

    void logMove(const CardCharacter& ch, const Move& move);
    void logWinner(const CardCharacter& ch);
    int decideLandlord();
    int nextPlayerIndex(int idx) const;

public:
    Game();
    ~Game();

    void initGame();
    void play();
};

#endif // GAME_H
