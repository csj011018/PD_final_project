#include "Game.h"
#include <iostream>
#include <random>
#include <chrono>

using namespace std;

Game::Game()
    : player("你"),
      landlordIndex(-1),
      currentPlayerIndex(0),
      lastMove(),
      lastMovePlayerIndex(-1),
      passCountInRound(0)
{
    enemies.emplace_back("AI_1");
    enemies.emplace_back("AI_2");

    playersAll.push_back(&player);
    playersAll.push_back(&enemies[0]);
    playersAll.push_back(&enemies[1]);

    items.emplace_back("對局紀錄卷軸");

    logFile.open("game_log.txt");
}

Game::~Game() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Game::logMove(const CardCharacter& ch, const Move& move) {
    if (!logFile.is_open()) return;
    logFile << "Player: " << ch.getName()
            << ", Type: " << handTypeToString(move.type)
            << ", Cards: ";
    for (const auto& c : move.cards) {
        logFile << c << " ";
    }
    logFile << "\n";
}

void Game::logWinner(const CardCharacter& ch) {
    if (!logFile.is_open()) return;
    logFile << "===== Winner: " << ch.getName() << " =====\n";
}

void Game::initGame() {
    deck.init();
    deck.shuffle();

    for (int i = 0; i < 17; ++i) {
        for (int p = 0; p < 3; ++p) {
            playersAll[p]->addCard(deck.draw());
        }
    }

    vector<Card> bottomCards;
    while (deck.size() > 0) {
        bottomCards.push_back(deck.draw());
    }

    for (auto p : playersAll) {
        p->sortHand();
    }

    cout << "\n=== 發牌完成，你的初始牌如下 ===\n";
    player.printHand();
    cout << endl;

    landlordIndex = decideLandlord();

    cout << ">>> 最終地主是 [" << playersAll[landlordIndex]->getName()
         << "]，獲得底牌：" << endl;
    for (const auto& c : bottomCards) {
        cout << c << "  ";
        playersAll[landlordIndex]->addCard(c);
    }
    cout << endl;

    playersAll[landlordIndex]->sortHand();

    currentPlayerIndex  = landlordIndex;
    lastMove            = Move();
    lastMovePlayerIndex = -1;
    passCountInRound    = 0;
}

int Game::decideLandlord() {
    char ans;
    cout << "【搶地主階段】\n";
    cout << "你要當地主嗎？請輸入 y 或 n，然後按 Enter：";
    cin >> ans;

    if (ans == 'y' || ans == 'Y') {
        cout << "你選擇當地主！\n";
        return 0;
    } else {
        unsigned seed = static_cast<unsigned>(
            chrono::system_clock::now().time_since_epoch().count()
        );
        default_random_engine eng(seed);
        uniform_int_distribution<int> dist(1, 2);
        int aiLandlord = dist(eng);
        cout << "你放棄當地主，系統隨機指定 "
             << playersAll[aiLandlord]->getName() << " 當地主。\n";
        return aiLandlord;
    }
}

int Game::nextPlayerIndex(int idx) const {
    return (idx + 1) % 3;
}

void Game::play() {
    cout << "==============================\n";
    cout << "   三人簡化大老二 / 鬥地主牌型混合版\n";
    cout << "   支援牌型：\n";
    cout << "     - 單張\n";
    cout << "     - 對子（AA）\n";
    cout << "     - 5 張順子（連號、不含 2 / Joker）\n";
    cout << "     - 葫蘆（AAA + BB 或 AA + BBB）\n";
    cout << "     - 炸彈（AAAA）\n";
    cout << "     - 火箭（兩張 Joker）\n";
    cout << "==============================\n\n";

    cout << "【遊戲流程說明】\n";
    cout << "1. 系統會發牌並詢問你是否要當地主（y/n）。\n";
    cout << "2. 地主拿到底牌後，從地主開始輪流出牌。\n";
    cout << "3. 有人手牌出完就獲勝，遊戲結束。\n\n";

    cout << "【出牌操作說明】\n";
    cout << "  - 每一輪會顯示你的手牌與上一手牌。\n";
    cout << "  - 出牌方式：\n";
    cout << "      請直接輸入要出的牌索引（用空白分隔）。\n";
    cout << "      例： 0 3 5\n";
    cout << "      若直接按 Enter（不輸入任何索引）表示 Pass。\n";
    cout << "  - 系統會自動判斷牌型並檢查是否能壓過上一手。\n\n";

    cout << "如果你看懂了規則，請輸入 1 然後按 Enter 開始遊戲：";
    int startFlag;
    cin >> startFlag;
    cout << "\n===== 遊戲開始！ =====\n\n";

    bool gameOver = false;

    while (!gameOver) {
        cout << "\n----------------------------------------\n";
        cout << "現在輪到 [" << playersAll[currentPlayerIndex]->getName() << "] 出牌。\n";

        Move currentMove = playersAll[currentPlayerIndex]->playTurn(lastMove);
        logMove(*playersAll[currentPlayerIndex], currentMove);

        if (currentMove.isPass()) {
            if (lastMove.type == HandType::Pass) {
                // 桌上本來就無牌
            } else {
                passCountInRound++;
                if (passCountInRound >= 2) {
                    cout << "已有兩位玩家連續 Pass，此輪結束，桌面清空。\n";
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

        if (playersAll[currentPlayerIndex]->isHandEmpty()) {
            cout << "\n============================\n";
            cout << "遊戲結束！贏家是 ["
                 << playersAll[currentPlayerIndex]->getName() << "]\n";
            cout << "============================\n";
            logWinner(*playersAll[currentPlayerIndex]);
            gameOver = true;
            break;
        }

        currentPlayerIndex = nextPlayerIndex(currentPlayerIndex);
    }
}
