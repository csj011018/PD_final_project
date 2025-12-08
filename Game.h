#ifndef GAME_H
#define GAME_H

#include <vector>
#include <fstream>
#include "Deck.h"
#include "Character.h"

class Game {
private:
    Deck deck;
    std::vector<CardCharacter*> players;
    std::ofstream logFile;

    int landlordIndex;
    int currentPlayerIndex;

    Move lastMove;
    int lastMovePlayerIndex;
    int passCountInRound;

public:
    Game();
    ~Game();

    void initGame();
    void play();

private:
    void logMove(const CardCharacter& player, const Move& move);
    void logWinner(const CardCharacter& player);

    int decideLandlord();
    int nextPlayerIndex(int idx) const;
};

#endif // GAME_H
