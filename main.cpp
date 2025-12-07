#include <iostream>
#include "Game.h"

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    try {
        Game game;
        game.initGame();
        game.play();
    } catch (const std::exception& e) {
        std::cerr << "程式發生例外：" << e.what() << std::endl;
    }

    return 0;
}
