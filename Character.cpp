#include "Character.h"

#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <map>
#include <sstream>
#include <random>

using namespace std;

// =====================
// CardCharacter
// =====================

CardCharacter::CardCharacter(const std::string& n)
    : name(n) {}

void CardCharacter::addCard(const Card& c) {
    hand.push_back(c);
}

void CardCharacter::sortHand() {
    std::sort(hand.begin(), hand.end());
}

void CardCharacter::printHand() const {
    cout << "Player [" << name << "] hand:\n";
    for (size_t i = 0; i < hand.size(); ++i) {
        cout << "(" << i << ") " << hand[i] << "  ";
    }
    cout << "\n";
}

const Card& CardCharacter::getCard(std::size_t index) const {
    if (index >= hand.size()) {
        throw std::out_of_range("Card index out of range");
    }
    return hand[index];
}

std::vector<Card> CardCharacter::playCardsByIndices(const std::vector<int>& indices) {
    if (indices.empty()) return {};

    std::vector<int> idx = indices;
    std::sort(idx.begin(), idx.end());

    std::vector<Card> chosen;
    chosen.reserve(idx.size());
    for (int i : idx) {
        if (i < 0 || static_cast<std::size_t>(i) >= hand.size()) {
            throw std::out_of_range("Card index out of range in playCardsByIndices");
        }
        chosen.push_back(hand[static_cast<std::size_t>(i)]);
    }

    for (int i = static_cast<int>(idx.size()) - 1; i >= 0; --i) {
        hand.erase(hand.begin() + idx[i]);
    }

    return chosen;
}

// =====================
// Player (human)
// =====================

static bool validateIndices(const CardCharacter& player,
                            const std::vector<int>& indices,
                            std::string& message)
{
    if (indices.empty()) {
        message = "No cards selected.";
        return false;
    }

    std::size_t nHand = player.handSize();

    for (std::size_t i = 0; i < indices.size(); ++i) {
        int idx = indices[i];
        if (idx < 0 || static_cast<std::size_t>(idx) >= nHand) {
            message = "Index " + std::to_string(idx) + " is out of range.";
            return false;
        }
        for (std::size_t j = i + 1; j < indices.size(); ++j) {
            if (indices[j] == indices[i]) {
                message = "Duplicate indices are not allowed.";
                return false;
            }
        }
    }
    return true;
}

Move Player::playTurnWithIndices(const Move& lastMove,
                                 const std::vector<int>& indices,
                                 bool& ok,
                                 std::string& message)
{
    ok = false;
    message.clear();

    if (indices.empty()) {
        // treat as Pass
        ok = true;
        return Move();
    }

    if (!validateIndices(*this, indices, message)) {
        return Move();
    }

    std::vector<Card> selected;
    selected.reserve(indices.size());
    for (int idx : indices) {
        selected.push_back(getCard(static_cast<std::size_t>(idx)));
    }

    auto info = analyzeHand(selected);
    HandType t = info.first;
    int mainRank = info.second;

    if (t == HandType::Invalid) {
        message = "This set of cards is not a valid hand type.";
        return Move();
    }

    Move candidate(t, selected, mainRank);

    if (!canBeat(lastMove, candidate)) {
        message = "This move cannot beat the previous move.";
        return Move();
    }

    std::vector<Card> played = playCardsByIndices(indices);
    (void)played; // not needed further here

    ok = true;
    return Move(t, selected, mainRank);
}

Move Player::playTurn(const Move& lastMove) {
    cout << "\n=== Your turn ===\n";
    printHand();

    cout << "Current table:\n";
    if (lastMove.type == HandType::Pass) {
        cout << "No cards on table. You may play any valid hand.\n";
    } else {
        cout << "Previous hand: [" << handTypeToString(lastMove.type) << "] ";
        for (const auto& c : lastMove.cards) {
            cout << c << "  ";
        }
        cout << "\n";
    }

    cout << "\nHow to play:\n";
    cout << " - Enter card indices separated by spaces, then press Enter.\n";
    cout << " - Example: 0 1 2\n";
    cout << " - Enter an empty line to Pass.\n\n";

    while (true) {
        cout << "Input indices (or empty line to Pass): ";
        std::string line;
        std::getline(std::cin, line);
        if (!std::cin) {
            std::cin.clear();
        }

        if (line.empty()) {
            cout << "You choose to Pass.\n";
            return Move();
        }

        std::vector<int> indices;
        {
            std::istringstream iss(line);
            int x;
            while (iss >> x) {
                indices.push_back(x);
            }
        }

        bool ok = false;
        std::string msg;
        Move mv = playTurnWithIndices(lastMove, indices, ok, msg);
        if (!ok) {
            cout << "Invalid move: " << msg << "\nPlease try again.\n";
            continue;
        }

        cout << "You play: [" << handTypeToString(mv.type) << "] ";
        for (const auto& c : mv.cards) cout << c << "  ";
        cout << "\n";
        return mv;
    }
}

// ======================
// Enemy (AI) smarter play
// ======================

bool Enemy::findSingleGreater(int targetRank, std::vector<int>& outIdx) const {
    const auto& h = hand;
    for (std::size_t i = 0; i < h.size(); ++i) {
        if (h[i].rank > targetRank) {
            outIdx = { static_cast<int>(i) };
            return true;
        }
    }
    return false;
}

bool Enemy::findPairGreater(int targetRank, std::vector<int>& outIdx) const {
    const auto& h = hand;
    std::size_t n = h.size();
    std::size_t i = 0;
    while (i + 1 < n) {
        int r = h[i].rank;
        std::size_t j = i + 1;
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

bool Enemy::findBomb(int targetRank, bool mustGreater, std::vector<int>& outIdx) const {
    const auto& h = hand;
    std::size_t n = h.size();
    std::size_t i = 0;
    while (i + 3 < n) {
        int r = h[i].rank;
        std::size_t j = i + 1;
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
                return true;
            }
        }
        i = j;
    }
    return false;
}

bool Enemy::findRocket(std::vector<int>& outIdx) const {
    const auto& h = hand;
    int idxSmall = -1, idxBig = -1;
    for (std::size_t i = 0; i < h.size(); ++i) {
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

bool Enemy::findStraightGreater(int targetHighRank, int length,
                                std::vector<int>& outIdx) const
{
    if (length != 5) return false;

    const auto& h = hand;
    std::map<int, int> rankToIdx;
    for (std::size_t i = 0; i < h.size(); ++i) {
        int r = h[i].rank;
        if (r < 3 || r > 14) continue; // no 2, no Jokers
        if (rankToIdx.find(r) == rankToIdx.end()) {
            rankToIdx[r] = static_cast<int>(i);
        }
    }
    if (rankToIdx.size() < static_cast<std::size_t>(length)) {
        return false;
    }

    std::vector<int> ranks;
    ranks.reserve(rankToIdx.size());
    for (auto& kv : rankToIdx) {
        ranks.push_back(kv.first);
    }

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

bool Enemy::findFullHouseGreater(int targetTripleRank,
                                 std::vector<int>& outIdx) const
{
    const auto& h = hand;
    std::map<int, std::vector<int>> rankToIdxList;
    for (std::size_t i = 0; i < h.size(); ++i) {
        rankToIdxList[h[i].rank].push_back(static_cast<int>(i));
    }

    for (auto itTriple = rankToIdxList.begin();
         itTriple != rankToIdxList.end(); ++itTriple)
    {
        int rTriple = itTriple->first;
        if (static_cast<int>(itTriple->second.size()) < 3) continue;
        if (rTriple <= targetTripleRank) continue;

        for (auto itPair = rankToIdxList.begin();
             itPair != rankToIdxList.end(); ++itPair)
        {
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

// --- helpers for opening a new round (lastMove == Pass) ---

// 找任意 5 張順子（盡量出「最小」的）
bool Enemy::findAnyStraight(int length, std::vector<int>& outIdx) const {
    if (length != 5) return false;

    const auto& h = hand;
    std::map<int, int> rankToIdx;
    for (std::size_t i = 0; i < h.size(); ++i) {
        int r = h[i].rank;
        if (r < 3 || r > 14) continue; // no 2, no Jokers
        if (rankToIdx.find(r) == rankToIdx.end()) {
            rankToIdx[r] = static_cast<int>(i);
        }
    }
    if (rankToIdx.size() < static_cast<std::size_t>(length)) {
        return false;
    }

    std::vector<int> ranks;
    ranks.reserve(rankToIdx.size());
    for (auto& kv : rankToIdx) {
        ranks.push_back(kv.first);
    }

    int bestHigh = 1000;
    std::vector<int> bestIndices;

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
        if (highRank < bestHigh) {
            bestHigh = highRank;
            bestIndices.clear();
            for (int k = 0; k < length; ++k) {
                int r = ranks[i + k];
                bestIndices.push_back(rankToIdx[r]);
            }
        }
    }

    if (bestIndices.empty()) return false;
    outIdx = bestIndices;
    return true;
}

// 找任意葫蘆（一樣選 triple 點數最小的）
bool Enemy::findAnyFullHouse(std::vector<int>& outIdx) const {
    const auto& h = hand;
    std::map<int, std::vector<int>> rankToIdxList;
    for (std::size_t i = 0; i < h.size(); ++i) {
        rankToIdxList[h[i].rank].push_back(static_cast<int>(i));
    }

    int bestTripleRank = 1000;
    std::vector<int> bestIndices;

    for (auto itTriple = rankToIdxList.begin();
         itTriple != rankToIdxList.end(); ++itTriple)
    {
        int rTriple = itTriple->first;
        if (static_cast<int>(itTriple->second.size()) < 3) continue;

        for (auto itPair = rankToIdxList.begin();
             itPair != rankToIdxList.end(); ++itPair)
        {
            int rPair = itPair->first;
            if (rPair == rTriple) continue;
            if (static_cast<int>(itPair->second.size()) < 2) continue;

            if (rTriple < bestTripleRank) {
                bestTripleRank = rTriple;
                bestIndices.clear();
                bestIndices.push_back(itTriple->second[0]);
                bestIndices.push_back(itTriple->second[1]);
                bestIndices.push_back(itTriple->second[2]);
                bestIndices.push_back(itPair->second[0]);
                bestIndices.push_back(itPair->second[1]);
            }
        }
    }

    if (bestIndices.empty()) return false;
    outIdx = bestIndices;
    return true;
}

// 開新一輪用的對子選擇：只選 rank <= 13（不會拿 AA / 22 / JokerJoker 開局）
bool Enemy::findOpeningPair(std::vector<int>& outIdx) const {
    const auto& h = hand;
    std::size_t n = h.size();
    std::size_t i = 0;
    while (i + 1 < n) {
        int r = h[i].rank;
        std::size_t j = i + 1;
        while (j < n && h[j].rank == r) {
            ++j;
        }
        int cnt = static_cast<int>(j - i);
        if (cnt >= 2) {
            // 避免 AA (14), 22 (15), Jokers(16,17)
            if (r <= 13) {
                outIdx = { static_cast<int>(i), static_cast<int>(i + 1) };
                return true;
            }
        }
        i = j;
    }
    return false;
}

// --- main AI decision ---

Move Enemy::playTurn(const Move& lastMove) {
    std::cout << "\n--- AI [" << name << "] turn ---\n";

    if (hand.empty()) {
        return Move(); // Pass
    }

    std::vector<int> idxList;
    std::vector<Card> played;
    std::pair<HandType, int> info;

    // 1) New round: lastMove is Pass
    if (lastMove.type == HandType::Pass) {
        bool found = false;

        // priority:
        //   5 cards (Straight > FullHouse) -> opening pair (low ranks) -> single
        if (findAnyStraight(5, idxList)) {
            found = true;
        } else if (findAnyFullHouse(idxList)) {
            found = true;
        } else if (findOpeningPair(idxList)) {
            found = true;
        } else if (findSingleGreater(-1, idxList)) { // -1 => smallest single
            found = true;
        }

        if (!found) {
            idxList = { 0 };
        }

        played = playCardsByIndices(idxList);
        info   = analyzeHand(played);

        std::cout << "AI [" << name << "] plays: ["
                  << handTypeToString(info.first) << "] ";
        for (const auto& c : played) std::cout << c << "  ";
        std::cout << "\n";

        return Move(info.first, played, info.second);
    }

    // 2) Following an existing move: try same type first
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
        found = findBomb(lastMove.mainRank, true, idxList);
    } else if (lastMove.type == HandType::Rocket) {
        found = false; // cannot beat rocket
    }

    if (found) {
        played = playCardsByIndices(idxList);
        info   = analyzeHand(played);
        std::cout << "AI [" << name << "] plays: ["
                  << handTypeToString(info.first) << "] ";
        for (const auto& c : played) std::cout << c << "  ";
        std::cout << "\n";
        return Move(info.first, played, info.second);
    }

    // 3) No same-type move: decide whether we are willing to use bomb / rocket
    bool allowBombRocket = true;
    if (bombDecisionProb < 1.0) {
        static std::mt19937 rng{ std::random_device{}() };
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        double r = dist(rng);
        if (r > bombDecisionProb) {
            allowBombRocket = false;
        }
    }

    if (allowBombRocket) {
        // 先試炸彈
        if (lastMove.type != HandType::Bomb && lastMove.type != HandType::Rocket) {
            if (findBomb(-1, false, idxList)) {
                played = playCardsByIndices(idxList);
                info   = analyzeHand(played);
                std::cout << "AI [" << name << "] plays: ["
                          << handTypeToString(info.first) << "] ";
                for (const auto& c : played) std::cout << c << "  ";
                std::cout << "\n";
                return Move(info.first, played, info.second);
            }
        }

        // 再試火箭
        if (findRocket(idxList)) {
            played = playCardsByIndices(idxList);
            info   = analyzeHand(played);
            std::cout << "AI [" << name << "] plays: ["
                      << handTypeToString(info.first) << "] ";
            for (const auto& c : played) std::cout << c << "  ";
            std::cout << "\n";
            return Move(info.first, played, info.second);
        }
    } else {
        std::cout << "AI [" << name << "] decides to save bomb/rocket.\n";
    }

    // 4) Really nothing or decided to keep bombs/rocket: Pass
    std::cout << "AI [" << name << "] chooses Pass.\n";
    return Move(); // Pass
}
