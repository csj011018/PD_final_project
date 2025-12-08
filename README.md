
---

# Landlord / Big Two – SFML Edition

A graphical implementation of a Landlord-style card game using **SFML** in C++17.
Players compete against two semi-smart AI opponents with visual card rendering, turn animation, and optional restart/end screens.

This README includes:

* Developer Guide (Setup, Compile, Run)
* Player Guide (Rules, Gameplay, UI)
* Project Structure & Notes

---

## 📁 Project Structure

```
/project_root
│── main_sfml.cpp               ← SFML GUI / Game loop
│── Game.cpp / Game.h           ← Game state & turn management
│── Character.cpp / Character.h ← Human & AI logic
│── Deck.cpp / Deck.h           ← Card dealing & shuffling
│── Card.cpp / Card.h           ← Card objects, hand types, comparison logic
│── assets/                     ← Fonts, images (optional)
│── README.md
```

---

# 🛠 Developer Guide

## ✔ Requirements

### ✔ macOS

* Homebrew
* SFML 3.x
  Install:

```
brew install sfml
```

### ✔ Compiler

Any compiler supporting **C++17**
(macOS Clang works perfectly)

---

## ✔ How to Compile

Run this command in the project directory:

```
g++ -std=c++17 main_sfml.cpp Game.cpp Character.cpp Deck.cpp Card.cpp \
    -o game_sfml \
    -I/opt/homebrew/include \
    -L/opt/homebrew/lib \
    -lsfml-graphics -lsfml-window -lsfml-system
```

If your SFML path differs, adjust `-I` and `-L`.

---

## ✔ How to Run

```
./game_sfml
```

If macOS blocks the executable, go to:

**System Settings → Privacy & Security → Allow Anyway**

---

## ✔ Developer Notes

### 1. Card logic (Card.cpp)

Handles:

* Hand type recognition
  (Single, Pair, Straight, Full House, Bomb, Rocket)
* Hand comparison rules
* Converting card data to display text

### 2. AI logic (Character.cpp)

AI supports:

* Prefers 5-card openings (Straight > Full House)
* Avoids playing strong pairs (2, Jokers) early
* Conditional Bomb/Rocket usage based on opponent hand size
* Attempts same-type response before using Bomb/Rocket

### 3. GUI Rendering (main_sfml.cpp)

Implements:

* Card layout system with screen-bounds auto spacing
* Click-to-select cards
* AI play animations (delayed actions)
* Display of left AI / right AI / human plays
* Turn indicators
* Restart & End screen

### 4. Assets

Place fonts in:

```
assets/
```

Modify in main_sfml.cpp:

```cpp
font.openFromFile("assets/YourFont.ttf");
```

---

# 🎮 Player Guide

## ✔ Game Flow

1. Start menu appears
2. Then the rule page
3. Player receives **17 cards**
4. Player chooses:

* **Become Landlord** → receives 3 extra cards
* **Pass** → a random AI becomes Landlord

Landlord always plays first.

---

## ✔ Controls

### 🂠 Selecting Cards

Click cards to select/unselect.
Selected cards move upward visually.

### ▶ Playing Cards

Click **Play**.

The game automatically detects:

* Single
* Pair
* Straight (5)
* Full House
* Bomb
* Rocket

If invalid, an error message appears.

### ↩ Passing

Click **Pass**.

A new round begins only when all opponents also pass.

---

# 🧠 AI Behavior

The AI follows multiple rules:

### When opening a round

Priority:

1. Best 5-card play (Straight → Full House)
2. Normal small pair
3. Smallest single
4. Avoids premium pairs (2, Joker) unless necessary

### When following a play

1. Try to beat using same type
2. If impossible → may use Bomb / Rocket
3. Probability-based bombing:

```
chance = 1 - (opponent_remaining_cards / opponent_initial_cards)
```

If chance is high, AI more likely to bomb or rocket.

---

# 🂡 Hand Types

| Type       | Description                       |
| ---------- | --------------------------------- |
| Single     | Any 1 card                        |
| Pair       | Two of same rank                  |
| Straight   | 5 consecutive ranks (no 2/Jokers) |
| Full House | Triple + Pair                     |
| Bomb       | Four of a kind                    |
| Rocket     | Both Jokers                       |

---

# 🏁 End of Game

When any player empties their hand:

* End screen appears (“Play Again?” / “Quit”)
* **Play Again** restarts with a new shuffle
* **Quit** exits the program

---

# ❗ Troubleshooting

### Cards show boxes/squares instead of suits

Use English-letter suit mode (C, D, H, S)
or install a font supporting Unicode suits.

### SFML errors on macOS

Ensure installation path is correct:

```
brew list sfml
```

Update include/library paths if needed.

### Random engine errors

Add to Character.cpp:

```cpp
#include <random>
```

---

# 🙌 Credits

Developed using:

* C++17
* SFML 3.x

If you want improvements like:

* Card animations
* Sound effects
* Smarter AI
* Online multiplayer

Feel free to request!

---
