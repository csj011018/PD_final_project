#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <random>

#include "Card.h"
#include "Character.h"
#include "Deck.h"

// ======================
// Layout constants
// ======================

constexpr float CARD_WIDTH   = 70.f;
constexpr float CARD_HEIGHT  = 90.f;
constexpr float CARD_GAP_MAX = 10.f;
constexpr float HUMAN_Y      = 520.f;
constexpr float SIDE_MARGIN  = 40.f;

// ======================
// Game state
// ======================

struct GuiGameState {
    Deck deck;
    Player human;
    Enemy ai1;
    Enemy ai2;
    std::vector<CardCharacter*> players;

    std::vector<Card> bottomCards;

    int landlordIndex        = 0;
    int currentPlayerIndex   = 0;

    Move lastMove;                  // last non-cleared table move（邏輯用，不再畫出）
    int  lastMovePlayerIndex = -1;
    int  passCountInRound    = 0;

    // 每個玩家最後一次行動（包括 Pass）
    Move lastAction[3];

    bool gameOver       = false;
    int  winnerIndex    = -1;

    bool landlordChosen = false;

    GuiGameState()
        : human("You"), ai1("AI_1"), ai2("AI_2")
    {
        players.push_back(&human);
        players.push_back(&ai1);
        players.push_back(&ai2);
        for (int i = 0; i < 3; ++i) {
            lastAction[i] = Move();
        }
    }
};

int nextIndex(int idx) { return (idx + 1) % 3; }

// ======================
// Card layout helpers
// ======================

struct CardLayout {
    float startX;
    float step;
};

CardLayout computeCardLayout(std::size_t count, float windowWidth) {
    if (count == 0) {
        return { windowWidth / 2.f, CARD_WIDTH + CARD_GAP_MAX };
    }
    if (count == 1) {
        float x = (windowWidth - CARD_WIDTH) / 2.f;
        return { x, CARD_WIDTH + CARD_GAP_MAX };
    }

    float available = windowWidth - 2.f * SIDE_MARGIN;
    if (available <= CARD_WIDTH) {
        return { SIDE_MARGIN, CARD_WIDTH };
    }

    float step = available / static_cast<float>(count - 1);
    float maxStep = CARD_WIDTH + CARD_GAP_MAX;
    if (step > maxStep) step = maxStep;

    float totalWidth = CARD_WIDTH + step * static_cast<float>(count - 1);
    float startX = (windowWidth - totalWidth) / 2.f;
    return { startX, step };
}

// ======================
// Game init / reset / applyMove
// ======================

void initGuiGame(GuiGameState& g) {
    g.deck.init();
    g.deck.shuffle();

    // 17 cards each
    for (int i = 0; i < 17; ++i) {
        for (int p = 0; p < 3; ++p) {
            g.players[p]->addCard(g.deck.draw());
        }
    }

    // 3 bottom cards
    g.bottomCards.clear();
    while (g.deck.size() > 0) {
        g.bottomCards.push_back(g.deck.draw());
    }

    for (auto* p : g.players) {
        p->sortHand();
    }

    g.landlordIndex        = 0;   // will be decided later
    g.currentPlayerIndex   = 0;
    g.lastMove             = Move();
    g.lastMovePlayerIndex  = -1;
    g.passCountInRound     = 0;
    g.gameOver             = false;
    g.winnerIndex          = -1;
    g.landlordChosen       = false;

    for (int i = 0; i < 3; ++i) {
        g.lastAction[i] = Move();
    }
}

// used when pressing R on game over
void resetFullGame(GuiGameState& g) {
    g.deck = Deck();
    g.bottomCards.clear();

    g.human = Player("You");
    g.ai1   = Enemy("AI_1");
    g.ai2   = Enemy("AI_2");

    g.players.clear();
    g.players.push_back(&g.human);
    g.players.push_back(&g.ai1);
    g.players.push_back(&g.ai2);

    initGuiGame(g);
}

void applyMove(GuiGameState& g, int playerIdx, const Move& mv) {
    // 記錄每個玩家最後一次行動（給畫面用）
    g.lastAction[playerIdx] = mv;

    if (mv.isPass()) {
        if (g.lastMove.type != HandType::Pass) {
            g.passCountInRound++;
            if (g.passCountInRound >= 2) {
                g.lastMove            = Move();
                g.lastMovePlayerIndex = -1;
                g.passCountInRound    = 0;
            }
        }
    } else {
        g.lastMove            = mv;
        g.lastMovePlayerIndex = playerIdx;
        g.passCountInRound    = 0;
    }

    if (g.players[playerIdx]->isHandEmpty()) {
        g.gameOver    = true;
        g.winnerIndex = playerIdx;
    } else {
        g.currentPlayerIndex = nextIndex(playerIdx);
    }
}

// ======================
// Drawing helpers
// ======================

void drawCard(sf::RenderWindow& window, sf::Font& font,
              const Card& card, int index,
              float x, float y, bool selected)
{
    sf::RectangleShape rect;
    rect.setSize({CARD_WIDTH, CARD_HEIGHT});
    rect.setPosition({x, y});
    rect.setFillColor(selected ?
        sf::Color(255, 255, 210) :
        sf::Color(230, 230, 230));
    rect.setOutlineColor(sf::Color::Black);
    rect.setOutlineThickness(2.f);

    if (index >= 0) {
        sf::Text idxText(font, "(" + std::to_string(index) + ")", 14);
        idxText.setFillColor(sf::Color::Blue);
        idxText.setPosition({x + 4.f, y + 2.f});
        window.draw(idxText);
    }

    std::string label = card.toString();
    sf::Text cardText(font, label, 20);
    cardText.setFillColor(sf::Color::Black);

    auto bounds = cardText.getLocalBounds();
    float textW = bounds.size.x;
    float textH = bounds.size.y;
    float tx = x + (CARD_WIDTH  - textW) / 2.f;
    float ty = y + (CARD_HEIGHT - textH) / 2.f;
    cardText.setPosition({tx, ty});

    window.draw(rect);
    window.draw(cardText);
}

void drawHumanHand(sf::RenderWindow& window,
                   const GuiGameState& g,
                   const std::vector<bool>& selected,
                   sf::Font& font)
{
    const auto& hand = g.human.getHand();
    std::size_t n = hand.size();
    if (n == 0) return;

    float windowWidth = static_cast<float>(window.getSize().x);
    CardLayout layout = computeCardLayout(n, windowWidth);

    for (std::size_t i = 0; i < n; ++i) {
        float x = layout.startX + layout.step * static_cast<float>(i);
        bool sel = (i < selected.size() && selected[i]);
        drawCard(window, font, hand[i], static_cast<int>(i), x, HUMAN_Y, sel);
    }
}

void drawAIPanels(sf::RenderWindow& window,
                  const GuiGameState& g,
                  sf::Font& font)
{
    for (int i = 1; i <= 2; ++i) {
        sf::Text t(font, "", 18);
        t.setFillColor(sf::Color::White);
        t.setPosition({50.f, static_cast<float>(i == 1 ? 40 : 70)});

        std::string s = g.players[i]->getNameRef() +
                        "   Cards: " +
                        std::to_string(g.players[i]->handSize());
        if (i == g.landlordIndex) {
            s += "   (Landlord)";
        }
        t.setString(s);
        window.draw(t);
    }
}

// 顯示三人的「最近一手」：左 AI1、中玩家、右 AI2
void drawActions(sf::RenderWindow& window,
                 const GuiGameState& g,
                 sf::Font& font)
{
    float w = static_cast<float>(window.getSize().x);
    float yLabel = 190.f;
    float yCards = 230.f;
    float leftXLabel  = SIDE_MARGIN;
    float rightXLabel = w - SIDE_MARGIN;
    float centerX     = w / 2.f;

    auto drawSide = [&](int idx, float labelX, bool leftSide) {
        const Move& mv = g.lastAction[idx];

        std::string header = g.players[idx]->getNameRef() + ": ";
        sf::Text h(font, "", 16);

        if (mv.type == HandType::Pass) {
            header += "PASS";
            h.setFillColor(sf::Color::Red);
        } else if (mv.type == HandType::Invalid) {
            header += "None";
            h.setFillColor(sf::Color::White);
        } else {
            header += handTypeToString(mv.type);
            h.setFillColor(sf::Color::White);
        }

        h.setString(header);
        auto hb = h.getLocalBounds();
        float hx = leftSide ? labelX : (labelX - hb.size.x);
        h.setPosition({hx, yLabel});
        window.draw(h);

        if (!mv.isPass() && mv.type != HandType::Invalid && !mv.cards.empty()) {
            const auto& cards = mv.cards;
            float step  = CARD_WIDTH + 5.f;
            float startX = leftSide ? labelX
                                    : (labelX - (CARD_WIDTH + step * static_cast<float>(cards.size() - 1)));

            for (std::size_t i = 0; i < cards.size(); ++i) {
                float cx = startX + step * static_cast<float>(i);
                drawCard(window, font, cards[i], -1, cx, yCards, false);
            }
        }
    };

    // 左右 AI
    drawSide(1, leftXLabel,  true);
    drawSide(2, rightXLabel, false);

    // 中間玩家
    const Move& mv = g.lastAction[0];
    std::string header = g.players[0]->getNameRef() + ": ";
    sf::Text h(font, "", 16);

    if (mv.type == HandType::Pass) {
        header += "PASS";
        h.setFillColor(sf::Color::Red);
    } else if (mv.type == HandType::Invalid) {
        header += "None";
        h.setFillColor(sf::Color::White);
    } else {
        header += handTypeToString(mv.type);
        h.setFillColor(sf::Color::White);
    }

    h.setString(header);
    auto hb = h.getLocalBounds();
    float hx = centerX - hb.size.x / 2.f;
    h.setPosition({hx, yLabel});
    window.draw(h);

    if (!mv.isPass() && mv.type != HandType::Invalid && !mv.cards.empty()) {
        const auto& cards = mv.cards;
        float step  = CARD_WIDTH + 5.f;
        float total = CARD_WIDTH + step * static_cast<float>(cards.size() - 1);
        float startX = centerX - total / 2.f;

        for (std::size_t i = 0; i < cards.size(); ++i) {
            float cx = startX + step * static_cast<float>(i);
            drawCard(window, font, cards[i], -1, cx, yCards, false);
        }
    }
}

void drawHUD(sf::RenderWindow& window,
             const GuiGameState& g,
             sf::Font& font,
             const std::string& err)
{
    // Turn 上移
    sf::Text info(font, "", 18);
    info.setFillColor(sf::Color::Yellow);
    info.setPosition({50.f, 110.f});

    if (!g.gameOver) {
        info.setString("Turn: " + g.players[g.currentPlayerIndex]->getNameRef());
    } else {
        info.setString("Game over! Winner: " +
                       g.players[g.winnerIndex]->getNameRef());
    }
    window.draw(info);

    // Controls 上移
    sf::Text help(font,
                  "Controls: click to select, Enter=Play, P=Pass, C=Clear",
                  16);
    help.setFillColor(sf::Color::Cyan);
    help.setPosition({50.f, 140.f});
    window.draw(help);

    if (!err.empty()) {
        sf::Text e(font, "Error: " + err, 16);
        e.setFillColor(sf::Color::Red);
        e.setPosition({50.f, 170.f});
        window.draw(e);
    }
}

void drawLandlordPrompt(sf::RenderWindow& window,
                        sf::Font& font)
{
    sf::Text title(font,
                   "Landlord selection",
                   24);
    title.setFillColor(sf::Color::Yellow);
    title.setPosition({50.f, 280.f});
    window.draw(title);

    sf::Text line1(font,
                   "Do you want to be the landlord? (Y = Yes, N = No)",
                   18);
    line1.setFillColor(sf::Color::White);
    line1.setPosition({50.f, 320.f});
    window.draw(line1);

    sf::Text line2(font,
                   "If you choose No, one AI will be randomly selected as landlord.",
                   16);
    line2.setFillColor(sf::Color(200, 200, 200));
    line2.setPosition({50.f, 350.f});
    window.draw(line2);
}

void drawEndScreen(sf::RenderWindow& window,
                   const GuiGameState& g,
                   sf::Font& font)
{
    sf::RectangleShape bg;
    bg.setSize(sf::Vector2f(
        static_cast<float>(window.getSize().x),
        static_cast<float>(window.getSize().y)));
    bg.setFillColor(sf::Color(0, 0, 0, 200));
    window.draw(bg);

    sf::Text title(font, "Game Over", 40);
    title.setFillColor(sf::Color::Yellow);

    auto tb = title.getLocalBounds();
    float tx = (window.getSize().x - tb.size.x) / 2.f;
    title.setPosition({tx, 150.f});
    window.draw(title);

    std::string winner = "Winner: " +
        g.players[g.winnerIndex]->getNameRef();
    sf::Text wtext(font, winner, 28);
    wtext.setFillColor(sf::Color::White);
    auto wb = wtext.getLocalBounds();
    float wx = (window.getSize().x - wb.size.x) / 2.f;
    wtext.setPosition({wx, 220.f});
    window.draw(wtext);

    sf::Text hint1(font,
                   "Press R to play again",
                   22);
    hint1.setFillColor(sf::Color::Cyan);
    auto h1b = hint1.getLocalBounds();
    float h1x = (window.getSize().x - h1b.size.x) / 2.f;
    hint1.setPosition({h1x, 280.f});
    window.draw(hint1);

    sf::Text hint2(font,
                   "Press Q to quit",
                   22);
    hint2.setFillColor(sf::Color::Cyan);
    auto h2b = hint2.getLocalBounds();
    float h2x = (window.getSize().x - h2b.size.x) / 2.f;
    hint2.setPosition({h2x, 320.f});
    window.draw(hint2);
}

// ======================
// Start / Rules screens
// ======================

void drawStartScreen(sf::RenderWindow& window, sf::Font& font)
{
    sf::Text title(font, "Doudizhu / Big Two", 40);
    title.setFillColor(sf::Color::Yellow);
    auto tb = title.getLocalBounds();
    float tx = (window.getSize().x - tb.size.x) / 2.f;
    title.setPosition({tx, 200.f});
    window.draw(title);

    sf::Text subtitle(font, "Simple 3-player landlord game", 22);
    subtitle.setFillColor(sf::Color::White);
    auto sb = subtitle.getLocalBounds();
    float sx = (window.getSize().x - sb.size.x) / 2.f;
    subtitle.setPosition({sx, 260.f});
    window.draw(subtitle);

    sf::Text hint(font,
                  "Press Enter to start, Q to quit",
                  22);
    hint.setFillColor(sf::Color::Cyan);
    auto hb = hint.getLocalBounds();
    float hx = (window.getSize().x - hb.size.x) / 2.f;
    hint.setPosition({hx, 320.f});
    window.draw(hint);
}

void drawRulesScreen(sf::RenderWindow& window, sf::Font& font)
{
    sf::Text title(font, "Rules (simplified)", 32);
    title.setFillColor(sf::Color::Yellow);
    auto tb = title.getLocalBounds();
    float tx = (window.getSize().x - tb.size.x) / 2.f;
    title.setPosition({tx, 80.f});
    window.draw(title);

    std::string rules =
        "1. Three players, one landlord, two farmers.\n"
        "2. Each player has 17 cards, plus 3 bottom cards for the landlord.\n"
        "3. Supported patterns:\n"
        "   - Single, Pair\n"
        "   - Straight of 5 cards (no 2 or Jokers)\n"
        "   - Full House (3 + 2)\n"
        "   - Bomb (4 of a kind)\n"
        "   - Rocket (two Jokers)\n"
        "4. Higher pattern of the same type can beat the previous play.\n"
        "   Bomb can beat any non-bomb, Rocket beats everything.\n"
        "\n"
        "Controls:\n"
        "   - Click cards to select / unselect\n"
        "   - Enter: play selected cards\n"
        "   - P: pass\n"
        "   - C: clear selection\n"
        "\n"
        "Press Enter to continue, Q to quit.";

    sf::Text body(font, rules, 20);
    body.setFillColor(sf::Color::White);
    body.setPosition({80.f, 140.f});
    window.draw(body);
}

// ======================
// Scene enum
// ======================

enum class Scene {
    Start,
    Rules,
    Game
};

// ======================
// main
// ======================

int main() {
    sf::RenderWindow window(
        sf::VideoMode(sf::Vector2u{1280u, 720u}),
        "Doudizhu GUI Demo"
    );
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("NotoSans-Regular.ttf")) {
        std::cerr << "WARNING: failed to load NotoSans-Regular.ttf\n";
    }

    GuiGameState game;
    initGuiGame(game);

    std::vector<bool> selected(game.human.handSize(), false);
    std::string errorMsg;

    sf::Clock clock;
    bool waitingForAI = false;
    float aiTriggerTime = 0.f;
    const float AI_DELAY = 0.8f;

    Scene scene = Scene::Start;

    while (window.isOpen()) {
        // -------- events --------
        while (auto ev = window.pollEvent()) {
            auto& e = *ev;

            if (e.is<sf::Event::Closed>()) {
                window.close();
                continue;
            }

            // Start screen
            if (scene == Scene::Start) {
                if (auto* k = e.getIf<sf::Event::KeyPressed>()) {
                    if (k->code == sf::Keyboard::Key::Enter ||
                        k->code == sf::Keyboard::Key::Space) {
                        scene = Scene::Rules;
                    } else if (k->code == sf::Keyboard::Key::Q ||
                               k->code == sf::Keyboard::Key::Escape) {
                        window.close();
                    }
                }
                continue;
            }

            // Rules screen
            if (scene == Scene::Rules) {
                if (auto* k = e.getIf<sf::Event::KeyPressed>()) {
                    if (k->code == sf::Keyboard::Key::Enter ||
                        k->code == sf::Keyboard::Key::Space) {
                        scene = Scene::Game;
                    } else if (k->code == sf::Keyboard::Key::Q ||
                               k->code == sf::Keyboard::Key::Escape) {
                        window.close();
                    }
                }
                continue;
            }

            // Game over: 只能 R / Q
            if (game.gameOver) {
                if (auto* k = e.getIf<sf::Event::KeyPressed>()) {
                    if (k->code == sf::Keyboard::Key::R) {
                        resetFullGame(game);
                        selected.assign(game.human.handSize(), false);
                        errorMsg.clear();
                        waitingForAI = false;
                        clock.restart();
                    } else if (k->code == sf::Keyboard::Key::Q ||
                               k->code == sf::Keyboard::Key::Escape) {
                        window.close();
                    }
                }
                continue;
            }

            // 地主選擇階段
            if (!game.landlordChosen) {
                if (auto* k = e.getIf<sf::Event::KeyPressed>()) {
                    if (k->code == sf::Keyboard::Key::Y) {
                        game.landlordIndex = 0;
                        for (const auto& c : game.bottomCards) {
                            game.human.addCard(c);
                        }
                        game.human.sortHand();
                        selected.assign(game.human.handSize(), false);
                        game.currentPlayerIndex = game.landlordIndex;
                        game.landlordChosen = true;
                        errorMsg.clear();
                    } else if (k->code == sf::Keyboard::Key::N) {
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_int_distribution<int> dist(1, 2);
                        int aiLandlord = dist(gen);

                        game.landlordIndex = aiLandlord;
                        for (const auto& c : game.bottomCards) {
                            dynamic_cast<Enemy*>(game.players[aiLandlord])->addCard(c);
                        }
                        game.players[aiLandlord]->sortHand();
                        game.currentPlayerIndex = game.landlordIndex;
                        game.landlordChosen = true;
                        waitingForAI = false;
                        errorMsg.clear();
                    }
                }
                continue;
            }

            // 正常遊戲階段：只有玩家回合才吃輸入
            if (!game.gameOver && game.currentPlayerIndex == 0) {
                if (auto* m = e.getIf<sf::Event::MouseButtonPressed>()) {
                    if (m->button == sf::Mouse::Button::Left) {
                        float mx = static_cast<float>(m->position.x);
                        float my = static_cast<float>(m->position.y);

                        const auto& hand = game.human.getHand();
                        std::size_t n = hand.size();
                        if (selected.size() < n) {
                            selected.resize(n, false);
                        }

                        float windowWidth = static_cast<float>(window.getSize().x);
                        CardLayout layout = computeCardLayout(n, windowWidth);

                        for (std::size_t i = 0; i < n; ++i) {
                            float x = layout.startX + layout.step * static_cast<float>(i);
                            float y = HUMAN_Y;
                            sf::FloatRect rect({x, y}, {CARD_WIDTH, CARD_HEIGHT});
                            if (rect.contains({mx, my})) {
                                selected[i] = !selected[i];
                            }
                        }
                    }
                }

                if (auto* k = e.getIf<sf::Event::KeyPressed>()) {
                    if (k->code == sf::Keyboard::Key::C) {
                        std::fill(selected.begin(), selected.end(), false);
                        errorMsg.clear();
                    }
                    else if (k->code == sf::Keyboard::Key::P) {
                        errorMsg.clear();
                        Move pass;
                        applyMove(game, 0, pass);
                        selected.assign(game.human.handSize(), false);
                        waitingForAI = false;
                    }
                    else if (k->code == sf::Keyboard::Key::Enter) {
                        errorMsg.clear();

                        const auto& hand = game.human.getHand();
                        if (selected.size() < hand.size()) {
                            selected.resize(hand.size(), false);
                        }

                        std::vector<int> idx;
                        for (std::size_t i = 0; i < hand.size(); ++i) {
                            if (selected[i]) {
                                idx.push_back(static_cast<int>(i));
                            }
                        }

                        bool ok = false;
                        std::string msg;
                        Move mv = game.human.playTurnWithIndices(
                            game.lastMove, idx, ok, msg
                        );

                        if (!ok) {
                            errorMsg = msg;
                        } else {
                            applyMove(game, 0, mv);
                            selected.assign(game.human.handSize(), false);
                            waitingForAI = false;
                        }
                    }
                }
            }
        }

        // -------- AI logic with delay --------
        if (scene == Scene::Game &&
            game.landlordChosen &&
            !game.gameOver &&
            game.currentPlayerIndex != 0)
        {
            float now = clock.getElapsedTime().asSeconds();
            if (!waitingForAI) {
                waitingForAI = true;
                aiTriggerTime = now + AI_DELAY;
            } else if (now >= aiTriggerTime) {
                Enemy* e = dynamic_cast<Enemy*>(game.players[game.currentPlayerIndex]);
                if (e) {
                    Move mv = e->playTurn(game.lastMove);
                    applyMove(game, game.currentPlayerIndex, mv);
                }
                waitingForAI = false;
            }
        }

        // -------- drawing --------
        window.clear(sf::Color(0, 100, 0));

        if (scene == Scene::Start) {
            drawStartScreen(window, font);
        } else if (scene == Scene::Rules) {
            drawRulesScreen(window, font);
        } else { // Game
            if (!game.gameOver) {
                drawHumanHand(window, game, selected, font);
                drawAIPanels(window, game, font);
                drawActions(window, game, font);
                drawHUD(window, game, font, errorMsg);

                if (!game.landlordChosen) {
                    drawLandlordPrompt(window, font);
                }
            } else {
                drawEndScreen(window, game, font);
            }
        }

        window.display();
    }

    return 0;
}
