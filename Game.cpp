#include "Game.h"

#include <iostream>
#include <chrono>
#include <random>

using namespace std;

Game::Game()
    : landlordIndex(-1),
      currentPlayerIndex(0),
      lastMove(),
      lastMovePlayerIndex(-1),
      passCountInRound(0)
{
    logFile.open("game_log.txt");
    players.push_back(new Player("You"));
    players.push_back(new Enemy("AI_1"));
    players.push_back(new Enemy("AI_2"));
}

Game::~Game() {
    for (auto p : players) {
        delete p;
    }
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Game::logMove(const CardCharacter& player, const Move& move) {
    if (!logFile.is_open()) return;
    logFile << "Player: " << player.getNameRef()
            << ", Type: " << handTypeToString(move.type)
            << ", Cards: ";
    for (const auto& c : move.cards) {
        logFile << c << " ";
    }
    logFile << "\n";
}

void Game::logWinner(const CardCharacter& player) {
    if (!logFile.is_open()) return;
    logFile << "===== Winner: " << player.getNameRef() << " =====\n";
}

int Game::nextPlayerIndex(int idx) const {
    return (idx + 1) % 3;
}

int Game::decideLandlord() {
    char ans;
    cout << "[Landlord selection]\n";
    cout << "Do you want to be the landlord? (y/n): ";
    cin >> ans;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (ans == 'y' || ans == 'Y') {
        cout << "You choose to be the landlord.\n";
        return 0;
    } else {
        unsigned seed = static_cast<unsigned>(
            chrono::system_clock::now().time_since_epoch().count()
        );
        default_random_engine eng(seed);
        uniform_int_distribution<int> dist(1, 2);
        int aiLandlord = dist(eng);
        cout << "You give up landlord. "
             << players[aiLandlord]->getNameRef()
             << " becomes the landlord.\n";
        return aiLandlord;
    }
}

void Game::initGame() {
    deck.init();
    deck.shuffle();

    for (int i = 0; i < 17; ++i) {
        for (int p = 0; p < 3; ++p) {
            players[p]->addCard(deck.draw());
        }
    }

    vector<Card> bottomCards;
    while (deck.size() > 0) {
        bottomCards.push_back(deck.draw());
    }

    for (auto p : players) {
        p->sortHand();
    }

    cout << "\n=== Initial hand (console mode) ===\n";
    players[0]->printHand();
    cout << endl;

    landlordIndex = decideLandlord();

    cout << ">>> Final landlord: [" << players[landlordIndex]->getNameRef()
         << "], extra cards: ";
    for (const auto& c : bottomCards) {
        cout << c << "  ";
        players[landlordIndex]->addCard(c);
    }
    cout << "\n";

    players[landlordIndex]->sortHand();

    currentPlayerIndex  = landlordIndex;
    lastMove            = Move();
    lastMovePlayerIndex = -1;
    passCountInRound    = 0;
}

void Game::play() {
    cout << "==============================\n";
    cout << "   Simplified Doudizhu (3 players)\n";
    cout << "   Supported hand types:\n";
    cout << "     - Single\n";
    cout << "     - Pair\n";
    cout << "     - Straight (5 cards, no 2/Joker)\n";
    cout << "     - Full House (AAA+BB or AA+BBB)\n";
    cout << "     - Bomb (AAAA)\n";
    cout << "     - Rocket (small Joker + big Joker)\n";
    cout << "==============================\n\n";

    cout << "Game flow:\n";
    cout << "1. System deals cards and decides landlord.\n";
    cout << "2. Landlord gets the 3 extra cards.\n";
    cout << "3. Players take turns to play. Empty input = Pass.\n";
    cout << "4. The first player to empty the hand wins.\n\n";

    cout << "Enter 1 and press Enter to start: ";
    int startFlag;
    cin >> startFlag;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "\n===== Game Start (console mode) =====\n\n";

    bool gameOver = false;

    while (!gameOver) {
        cout << "\n----------------------------------------\n";
        cout << "Now it is [" << players[currentPlayerIndex]->getNameRef()
             << "]'s turn.\n";

        Move currentMove = players[currentPlayerIndex]->playTurn(lastMove);
        logMove(*players[currentPlayerIndex], currentMove);

        if (currentMove.isPass()) {
            if (lastMove.type != HandType::Pass) {
                passCountInRound++;
                if (passCountInRound >= 2) {
                    cout << "Two consecutive Pass. Round cleared.\n";
                    lastMove            = Move();
                    lastMovePlayerIndex = -1;
                    passCountInRound    = 0;
                }
            }
        } else {
            lastMove            = currentMove;
            lastMovePlayerIndex = currentPlayerIndex;
            passCountInRound    = 0;
        }

        if (players[currentPlayerIndex]->isHandEmpty()) {
            cout << "\n============================\n";
            cout << "Game over! Winner: ["
                 << players[currentPlayerIndex]->getNameRef() << "]\n";
            cout << "============================\n";
            logWinner(*players[currentPlayerIndex]);
            gameOver = true;
            break;
        }

        currentPlayerIndex = nextPlayerIndex(currentPlayerIndex);
    }
}
