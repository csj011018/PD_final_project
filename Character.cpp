#include "Character.h"
#include <iostream>
#include <algorithm>
#include <map>
#include <limits>
#include <sstream>
#include <cctype>

using namespace std;

// ================= Character / Item =================

Character::Character(const std::string& n, int hp)
    : name(n), health(hp) {}

Character::~Character() = default;

std::string Character::getName() const {
    return name;
}

int Character::getHealth() const {
    return health;
}

Item::Item(const std::string& n)
    : name(n) {}

std::string Item::getName() const {
    return name;
}

// ================= CardCharacter =================

CardCharacter::CardCharacter(const std::string& n, int hp)
    : Character(n, hp) {}

CardCharacter::~CardCharacter() = default;

const std::string& CardCharacter::getNameRef() const {
    return name;
}

void CardCharacter::addCard(const Card& c) {
    hand.push_back(c);
}

void CardCharacter::sortHand() {
    std::sort(hand.begin(), hand.end());
}

void CardCharacter::printHand() const {
    cout << "玩家 [" << name << "] 的手牌：" << endl;
    for (size_t i = 0; i < hand.size(); ++i) {
        cout << "(" << i << ") " << hand[i] << "  ";
    }
    cout << endl;
}

size_t CardCharacter::handSize() const {
    return hand.size();
}

bool CardCharacter::isHandEmpty() const {
    return hand.empty();
}

const Card& CardCharacter::getCard(size_t index) const {
    if (index >= hand.size()) {
        throw out_of_range("Card index out of range.");
    }
    return hand[index];
}

vector<Card> CardCharacter::playCardsByIndices(const vector<int>& indices) {
    if (indices.empty()) return {};

    vector<int> idx = indices;
    sort(idx.begin(), idx.end());
    vector<Card> chosen;
    chosen.reserve(idx.size());
    for (int i : idx) {
        if (i < 0 || static_cast<size_t>(i) >= hand.size()) {
            throw out_of_range("Card index out of range in playCardsByIndices.");
        }
        chosen.push_back(hand[static_cast<size_t>(i)]);
    }
    for (int i = static_cast<int>(idx.size()) - 1; i >= 0; --i) {
        hand.erase(hand.begin() + idx[i]);
    }
    return chosen;
}

const vector<Card>& CardCharacter::getHand() const {
    return hand;
}

// ================= Player =================

Player::Player(const std::string& n)
    : CardCharacter(n, 100) {}

void Player::attack(Character& target) {
    cout << "玩家 [" << name << "] 對 [" << target.getName()
         << "] 發動攻擊（在本遊戲中代表出牌行為）。\n";
}

Move Player::playTurn(const Move& lastMove) {
    while (true) {
        cout << "\n=== 輪到你出牌 ===\n";
        printHand();

        cout << "【目前桌面狀態】\n";
        if (lastMove.type == HandType::Pass) {
            cout << "桌面目前沒有牌，你可以出任何合法牌型。\n";
        } else {
            cout << "上一手牌型：" << handTypeToString(lastMove.type) << "，牌：";
            for (const auto& c : lastMove.cards) {
                cout << c << "  ";
            }
            cout << "\n";
        }

        cout << "請輸入要出的牌索引（用空白分隔，直接按 Enter = Pass）：\n";

        if (cin.peek() == '\n') {
            cin.get();
        }

        string line;
        if (!getline(cin, line)) {
            cout << "讀取輸入失敗，請重新出牌。\n";
            cin.clear();
            continue;
        }

        bool allSpace = true;
        for (char ch : line) {
            if (!isspace(static_cast<unsigned char>(ch))) {
                allSpace = false;
                break;
            }
        }

        if (allSpace) {
            cout << "你選擇 Pass。\n";
            return Move();
        }

        istringstream iss(line);
        vector<int> idxList;
        int idx;
        while (iss >> idx) {
            idxList.push_back(idx);
        }

        if (idxList.empty()) {
            cout << "沒有讀到任何有效索引，請重新輸入。\n";
            continue;
        }

        size_t n = idxList.size();

        for (size_t i = 0; i < n; ++i) {
            int idx2 = idxList[i];
            if (idx2 < 0 || static_cast<size_t>(idx2) >= handSize()) {
                cout << "索引 " << idx2 << " 超出範圍，請重新出牌。\n";
                goto CONTINUE_INPUT;
            }
            for (size_t j = i + 1; j < n; ++j) {
                if (idxList[j] == idxList[i]) {
                    cout << "索引 " << idx2 << " 重複輸入，請重新出牌。\n";
                    goto CONTINUE_INPUT;
                }
            }
        }

        {
            vector<Card> selected;
            selected.reserve(n);
            for (int id : idxList) {
                selected.push_back(getCard(static_cast<size_t>(id)));
            }

            auto info = analyzeHand(selected);
            HandType t = info.first;
            int mainRank = info.second;

            if (t == HandType::Invalid) {
                cout << "這組牌不是合法牌型（單張 / 對子 / 5 張順子 / 葫蘆 / 炸彈 / 火箭），請重新出牌。\n";
                goto CONTINUE_INPUT;
            }

            Move candidate(t, selected, mainRank);

            if (!canBeat(lastMove, candidate)) {
                cout << "這組牌無法壓過上一手牌，請重新選擇。\n";
                goto CONTINUE_INPUT;
            }

            vector<Card> played = playCardsByIndices(idxList);
            cout << "你出牌：[" << handTypeToString(t) << "] ";
            for (const auto& c : played) {
                cout << c << "  ";
            }
            cout << "\n";

            return Move(t, played, mainRank);
        }

    CONTINUE_INPUT:
        continue;
    }
}

// ================= Enemy（AI） =================

Enemy::Enemy(const std::string& n)
    : CardCharacter(n, 100) {}

void Enemy::attack(Character& target) {
    cout << "敵人 [" << name << "] 對 [" << target.getName()
         << "] 發動攻擊（在本遊戲中代表 AI 出牌）。\n";
}

// ---- 以下都是 AI 用的小工具 ----

bool Enemy::findSingleGreater(int targetRank, vector<int>& outIdx) {
    const auto& h = getHand();
    for (size_t i = 0; i < h.size(); ++i) {
        if (h[i].rank > targetRank) {
            outIdx = { static_cast<int>(i) };
            return true;
        }
    }
    return false;
}

bool Enemy::findPairGreater(int targetRank, vector<int>& outIdx) {
    const auto& h = getHand();
    size_t n = h.size();
    size_t i = 0;
    while (i + 1 < n) {
        int r = h[i].rank;
        size_t j = i + 1;
        while (j < n && h[j].rank == r) {
            ++j;
        }
        int cnt = static_cast<int>(j - i);
        if (cnt >= 2 && r > targetRank) {
            outIdx = { static_cast<int>(i), static_cast<int>(i + 1) };
            return true;
        }
        i = j;
    }
    return false;
}

bool Enemy::findBomb(int targetRank, bool mustGreater, vector<int>& outIdx, int& bombRank) {
    const auto& h = getHand();
    size_t n = h.size();
    size_t i = 0;
    while (i + 3 < n) {
        int r = h[i].rank;
        size_t j = i + 1;
        while (j < n && h[j].rank == r) {
            ++j;
        }
        int cnt = static_cast<int>(j - i);
        if (cnt >= 4) {
            if (!mustGreater || r > targetRank) {
                outIdx = {
                    static_cast<int>(i),
                    static_cast<int>(i + 1),
                    static_cast<int>(i + 2),
                    static_cast<int>(i + 3)
                };
                bombRank = r;
                return true;
            }
        }
        i = j;
    }
    return false;
}

bool Enemy::findRocket(vector<int>& outIdx) {
    const auto& h = getHand();
    int idxSmall = -1, idxBig = -1;
    for (size_t i = 0; i < h.size(); ++i) {
        if (h[i].suit == Suit::Joker) {
            if (h[i].rank == 16) idxSmall = static_cast<int>(i);
            if (h[i].rank == 17) idxBig   = static_cast<int>(i);
        }
    }
    if (idxSmall != -1 && idxBig != -1) {
        outIdx = { idxSmall, idxBig };
        return true;
    }
    return false;
}

bool Enemy::findStraightGreater(int targetHighRank, int length, vector<int>& outIdx) {
    if (length != 5) return false;
    const auto& h = getHand();

    map<int, int> rankToIdx;
    for (size_t i = 0; i < h.size(); ++i) {
        int r = h[i].rank;
        if (r < 3 || r > 14) continue;
        if (rankToIdx.find(r) == rankToIdx.end()) {
            rankToIdx[r] = static_cast<int>(i);
        }
    }

    if (rankToIdx.size() < static_cast<size_t>(length)) {
        return false;
    }

    vector<int> ranks;
    ranks.reserve(rankToIdx.size());
    for (auto& kv : rankToIdx) {
        ranks.push_back(kv.first);
    }

    sort(ranks.begin(), ranks.end());
    for (int i = 0; i + length - 1 < static_cast<int>(ranks.size()); ++i) {
        bool ok = true;
        for (int j = 0; j + 1 < length; ++j) {
            if (ranks[i + j + 1] != ranks[i + j] + 1) {
                ok = false;
                break;
            }
        }
        if (!ok) continue;
        int highRank = ranks[i + length - 1];
        if (highRank > targetHighRank) {
            outIdx.clear();
            for (int k = 0; k < length; ++k) {
                int r = ranks[i + k];
                outIdx.push_back(rankToIdx[r]);
            }
            return true;
        }
    }
    return false;
}

bool Enemy::findFullHouseGreater(int targetTripleRank, vector<int>& outIdx) {
    const auto& h = getHand();
    map<int, vector<int>> rankToIdxList;
    for (size_t i = 0; i < h.size(); ++i) {
        rankToIdxList[h[i].rank].push_back(static_cast<int>(i));
    }

    for (auto itTriple = rankToIdxList.begin(); itTriple != rankToIdxList.end(); ++itTriple) {
        int rTriple = itTriple->first;
        if (static_cast<int>(itTriple->second.size()) < 3) continue;
        if (rTriple <= targetTripleRank) continue;

        for (auto itPair = rankToIdxList.begin(); itPair != rankToIdxList.end(); ++itPair) {
            int rPair = itPair->first;
            if (rPair == rTriple) continue;
            if (static_cast<int>(itPair->second.size()) < 2) continue;

            outIdx.clear();
            outIdx.push_back(itTriple->second[0]);
            outIdx.push_back(itTriple->second[1]);
            outIdx.push_back(itTriple->second[2]);
            outIdx.push_back(itPair->second[0]);
            outIdx.push_back(itPair->second[1]);
            return true;
        }
    }
    return false;
}

bool Enemy::findAnyPair(vector<int>& outIdx) {
    const auto& h = getHand();
    size_t n = h.size();
    size_t i = 0;
    while (i + 1 < n) {
        int r = h[i].rank;
        size_t j = i + 1;
        while (j < n && h[j].rank == r) {
            ++j;
        }
        int cnt = static_cast<int>(j - i);
        if (cnt >= 2) {
            outIdx = { static_cast<int>(i), static_cast<int>(i + 1) };
            return true;
        }
        i = j;
    }
    return false;
}

bool Enemy::findAnyStraightLen5(vector<int>& outIdx) {
    const auto& h = getHand();
    map<int, int> rankToIdx;
    for (size_t i = 0; i < h.size(); ++i) {
        int r = h[i].rank;
        if (r < 3 || r > 14) continue;
        if (rankToIdx.find(r) == rankToIdx.end()) {
            rankToIdx[r] = static_cast<int>(i);
        }
    }
    if (rankToIdx.size() < 5) return false;

    vector<int> ranks;
    ranks.reserve(rankToIdx.size());
    for (auto& kv : rankToIdx) {
        ranks.push_back(kv.first);
    }

    sort(ranks.begin(), ranks.end());
    for (int i = 0; i + 4 < static_cast<int>(ranks.size()); ++i) {
        bool ok = true;
        for (int j = 0; j < 4; ++j) {
            if (ranks[i + j + 1] != ranks[i + j] + 1) {
                ok = false;
                break;
            }
        }
        if (!ok) continue;

        outIdx.clear();
        for (int k = 0; k < 5; ++k) {
            int r = ranks[i + k];
            outIdx.push_back(rankToIdx[r]);
        }
        return true;
    }
    return false;
}

bool Enemy::findAnyFullHouse(vector<int>& outIdx) {
    const auto& h = getHand();
    map<int, vector<int>> rankToIdxList;
    for (size_t i = 0; i < h.size(); ++i) {
        rankToIdxList[h[i].rank].push_back(static_cast<int>(i));
    }

    int bestTripleRank = 100;
    int chosenTripleRank = -1;
    int chosenPairRank = -1;

    for (auto& kvT : rankToIdxList) {
        int rTriple = kvT.first;
        if (static_cast<int>(kvT.second.size()) < 3) continue;
        if (rTriple >= bestTripleRank) continue;

        for (auto& kvP : rankToIdxList) {
            int rPair = kvP.first;
            if (rPair == rTriple) continue;
            if (static_cast<int>(kvP.second.size()) < 2) continue;

            bestTripleRank = rTriple;
            chosenTripleRank = rTriple;
            chosenPairRank = rPair;
        }
    }

    if (chosenTripleRank == -1) return false;

    outIdx.clear();
    auto& tIdx = rankToIdxList[chosenTripleRank];
    auto& pIdx = rankToIdxList[chosenPairRank];
    outIdx.push_back(tIdx[0]);
    outIdx.push_back(tIdx[1]);
    outIdx.push_back(tIdx[2]);
    outIdx.push_back(pIdx[0]);
    outIdx.push_back(pIdx[1]);
    return true;
}

bool Enemy::findLeadSingle(vector<int>& outIdx) {
    const auto& h = getHand();
    int idxBest = -1;
    int bestRank = 100;

    for (size_t i = 0; i < h.size(); ++i) {
        int r = h[i].rank;
        if (r >= 15) continue;
        if (r < bestRank) {
            bestRank = r;
            idxBest = static_cast<int>(i);
        }
    }

    if (idxBest == -1) {
        if (h.empty()) return false;
        idxBest = 0;
    }

    outIdx = { idxBest };
    return true;
}

Move Enemy::playTurn(const Move& lastMove) {
    cout << "\n--- 輪到 AI [" << name << "] 出牌 ---\n";

    if (isHandEmpty()) {
        return Move();
    }

    int cardsBefore = static_cast<int>(handSize());

    vector<int> idxList;
    vector<Card> played;
    pair<HandType, int> info;

    if (lastMove.type == HandType::Pass) {
        if (findAnyStraightLen5(idxList)) {
            played = playCardsByIndices(idxList);
            info   = analyzeHand(played);
        }
        else if (findAnyPair(idxList)) {
            played = playCardsByIndices(idxList);
            info   = analyzeHand(played);
        }
        else if (findLeadSingle(idxList)) {
            played = playCardsByIndices(idxList);
            info   = analyzeHand(played);
        }
        else {
            cout << "AI [" << name << "] 找不到可以領先的牌，Pass。\n";
            return Move();
        }

        cout << "AI [" << name << "] 領先出牌：[" << handTypeToString(info.first) << "] ";
        for (const auto& c : played) cout << c << "  ";
        cout << "\n";
        return Move(info.first, played, info.second);
    }

    bool found = false;

    if (lastMove.type == HandType::Single) {
        found = findSingleGreater(lastMove.mainRank, idxList);
    } else if (lastMove.type == HandType::Pair) {
        found = findPairGreater(lastMove.mainRank, idxList);
    } else if (lastMove.type == HandType::Straight) {
        int len = static_cast<int>(lastMove.cards.size());
        found = findStraightGreater(lastMove.mainRank, len, idxList);
    } else if (lastMove.type == HandType::FullHouse) {
        found = findFullHouseGreater(lastMove.mainRank, idxList);
    } else if (lastMove.type == HandType::Bomb) {
        int dummyRank = -1;
        found = findBomb(lastMove.mainRank, true, idxList, dummyRank);
    } else if (lastMove.type == HandType::Rocket) {
        found = false;
    }

    if (found) {
        played = playCardsByIndices(idxList);
        info   = analyzeHand(played);
        cout << "AI [" << name << "] 出牌：[" << handTypeToString(info.first) << "] ";
        for (const auto& c : played) cout << c << "  ";
        cout << "\n";
        return Move(info.first, played, info.second);
    }

    if (lastMove.type != HandType::Bomb && lastMove.type != HandType::Rocket) {
        int bombRank = -1;
        if (findBomb(-1, false, idxList, bombRank)) {
            bool useBomb = false;

            if (cardsBefore <= 8) useBomb = true;
            if (lastMove.mainRank >= 14) useBomb = true;

            if (useBomb) {
                played = playCardsByIndices(idxList);
                info   = analyzeHand(played);
                cout << "AI [" << name << "] 使用炸彈：";
                for (const auto& c : played) cout << c << "  ";
                cout << "\n";
                return Move(info.first, played, info.second);
            }
        }
    }

    {
        vector<int> rocketIdx;
        if (findRocket(rocketIdx)) {
            bool useRocket = false;
            if (cardsBefore <= 6) useRocket = true;
            if (lastMove.type == HandType::Bomb || lastMove.type == HandType::Rocket) {
                useRocket = true;
            }

            if (useRocket) {
                played = playCardsByIndices(rocketIdx);
                info   = analyzeHand(played);
                cout << "AI [" << name << "] 使用火箭：";
                for (const auto& c : played) cout << c << "  ";
                cout << "\n";
                return Move(info.first, played, info.second);
            }
        }
    }

    cout << "AI [" << name << "] 選擇 Pass。\n";
    return Move();
}
