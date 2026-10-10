# DungeonForge

> A 2D top-down roguelike written entirely in **C (C17)** with [raylib](https://www.raylib.com/), featuring procedural dungeons, A* pathfinding, a boss fight, and enemies that **adapt to how you play**.

![Language](https://img.shields.io/badge/language-C17-blue)
![Library](https://img.shields.io/badge/graphics-raylib%206.0-red)
![Build](https://img.shields.io/badge/build-GNU%20Make-green)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

---

## Table of Contents

1. [Project Proposal](#project-proposal)
2. [Features](#features)
3. [Adaptive Enemy Intelligence](#adaptive-enemy-intelligence)
4. [Controls](#controls)
5. [Build and Run](#build-and-run)
6. [Testing](#testing)
7. [Project Structure](#project-structure)
8. [Design and Architecture](#design-and-architecture)
9. [Known Limitations and Roadmap](#known-limitations-and-roadmap)
10. [License](#license)

---

## Project Proposal

### Description

DungeonForge is a real-time 2D roguelike. The player explores a procedurally generated dungeon, fights enemies, collects health potions, and defeats a multi-phase boss. The project is written in C only, and every system (game loop, collision, pathfinding, AI, save/load, event queue) is implemented from scratch on top of raylib's windowing, drawing, and audio functions.

The distinguishing feature is **Adaptive Enemy Intelligence**: the game records how the player moves, dodges, and attacks, builds a statistical profile, and lets enemies choose counter-strategies from that profile. No machine learning is used, only counters, probabilities, and decision rules.

### Goals

- Demonstrate structured, modular programming in C: headers/sources separated, clear module interfaces, no global-state sprawl.
- Implement classic game-programming algorithms by hand: tile collision, A* pathfinding, finite-state machines, procedural generation.
- Build an event-driven analysis pipeline that influences gameplay.
- Persist game state with binary serialization and a versioned save format.
- Verify the logic-heavy modules with automated tests.
- Follow good repository practice: `.gitignore`, `Makefile`, small meaningful commits, license, documentation.

### Specifications

| Item | Detail |
|---|---|
| Language | C17 (`-std=c17 -Wall -Wextra`) |
| Graphics / audio | raylib 6.0 (Windows, MinGW-w64) |
| Build system | GNU Make |
| Window | 1280 x 720, 60 FPS |
| World | 40 x 22 tile map, 32 px tiles |
| Dungeon | Up to 20 rooms, spanning-tree connectivity, 2-tile-wide corridors |
| Enemies | Hunter, Guardian, Assassin, plus a 3-phase Boss |
| Pathfinding | 8-direction A* with no corner cutting |
| Save format | Binary, magic number + version header |

### Design Summary

The game is a set of small modules coordinated by `game.c`. Gameplay code pushes **events** into a fixed-size queue; the **behavior** module turns events into a player profile; the **adaptive AI** module turns that profile into per-enemy decisions; the **enemy FSM** acts on those decisions. See [Design and Architecture](#design-and-architecture).

---

## Features

- Top-down gameplay with 8-direction movement and a dodge roll with invulnerability frames
- Real-time melee combat (cone-shaped attack)
- Procedural dungeon generation (rooms, corridors, dungeon graph)
- Enemy finite-state machines (idle, chase, attack, hurt, dead)
- A* pathfinding with safe waypoints and unreachable-goal handling
- Local steering and enemy separation to avoid jitter and overlap
- Boss with three health phases, a special attack, and recovery states
- Inventory (12 slots), item drops, and health potions
- Binary save/load, including the learned player profile
- HUD, boss health bar, debug overlays, sound effects
- Automated tests for the event queue, behavior profile, adaptive AI, serialization, and pathfinding

---

## Adaptive Enemy Intelligence

During a run, the game observes:

- **Movement direction** (left / right / up / down)
- **Dodge direction**
- **Attack count**
- Damage taken and healing (collected for the profile)

Each enemy asks the AI for a decision about once per second. A decision only takes effect when a probability roll succeeds, so enemies are not perfectly predictable.

| Archetype | Uses | Behavior |
|---|---|---|
| **Hunter** | Dodge habits (movement as fallback) | Predicts the player's likely dodge direction and aims slightly ahead of it |
| **Guardian** | Attack frequency | Changes its attack tempo depending on how aggressive the player is |
| **Assassin** | Dodge habits (movement as fallback) | Flanks the side opposite the player's dodge habit |
| **Boss** | Dodge habits (movement as fallback) | Flanks or closes distance based on the player's tendencies |

The learned profile is stored in the save file, so loading a game keeps what the enemies have learned.

---

## Controls

| Key | Action |
|---|---|
| `W` `A` `S` `D` | Move |
| `Shift` | Dodge |
| `Space` | Melee attack |
| `H` | Drink health potion |
| `R` | Restart after death or victory |
| `F3` | Toggle enemy debug text |
| `F4` | Toggle dungeon graph overlay |
| `F5` | Toggle A* pathfinding overlay |
| `F6` | Save game |
| `F7` | Load game |

---

## Build and Run

### Prerequisites

- Windows with **MinGW-w64** (GCC) and **GNU Make** (for example via MSYS2 or Git Bash with `make`)
- **raylib 6.0 for Windows / MinGW-w64**

### Steps

1. Clone the repository:
   ```bash
   git clone <your-repository-url>
   cd DungeonForge
   ```
2. Download `raylib-6.0_win64_mingw-w64` from the [raylib releases page](https://github.com/raysan5/raylib/releases) and extract it so the folder is:
   ```
   third_party/raylib-6.0_win64_mingw-w64/
   ├── include/   (already in the repository)
   └── lib/       (contains libraylib.a, from the download)
   ```
   The repository includes the raylib headers; the compiled library (`.a`) is not committed.
3. Build:
   ```bash
   make
   ```
4. Run from the project root (the game loads `assets/audio/...` by relative path):
   ```bash
   ./dungeonforge
   ```

### Other commands

```bash
make test    # build and run all test suites
make clean   # remove build outputs
```

---

## Testing

```bash
make test
```

| Suite | What it checks |
|---|---|
| `test_events` | FIFO order, capacity, wrap-around, empty queue, NULL handling |
| `test_behavior` | Counters, probabilities, damage/heal statistics, profile restore and validation |
| `test_adaptive_ai` | Decision validity, per-archetype behavior, probabilistic adaptation |
| `test_serialization` | Round trip, truncated files, invalid arguments |
| `test_pathfinding` | Valid/invalid inputs, same-tile and cross-room paths, unreachable goals |

The tests cover the logic modules that do not need a window. Rendering, collision, player, and FSM code are verified by playing the game.

---

## Project Structure

```
DungeonForge/
├── Makefile
├── README.md
├── LICENSE
├── .gitignore
├── assets/audio/        sound effects (.ogg)
├── include/             public headers (.h)
├── src/                 implementation (.c)
├── tests/               automated test programs
├── saves/               save-game directory
└── third_party/         raylib headers (library downloaded separately)
```

---

## Design and Architecture

```text
Input
  │
  ▼
Game (game.c)  ── main loop, HUD, save/load keys
  │
  ├── World:    TileMap ◄── Dungeon (rooms, corridors, graph)
  ├── Entities: Player, Enemies (pool of 32), Item drops
  ├── Combat:   Player melee cone, enemy attacks
  ├── Movement: Collision, A* Pathfinding, local steering
  ├── Enemy FSM: idle → chase → attack → hurt → dead  (+ Boss states)
  │
  └── Adaptive AI pipeline
        Gameplay ──events──► Event Queue
                                  │
                                  ▼
                          Behavior Profile
                                  │
                                  ▼
                         Adaptive AI decision
                                  │
                                  ▼
                              Enemy FSM
```

Key design decisions:

- **Fixed-size data structures** (event ring buffer of 256, enemy pool of 32, drop pool of 32): no dynamic allocation in the game loop.
- **Event-driven analysis**: gameplay code only pushes events and never touches the AI, so the AI can be tested with no window.
- **Deterministic dungeons**: the dungeon is regenerated from a seed, so saves store the seed and not the whole map.
- **Versioned saves**: a magic number and version reject incompatible or foreign files.
- **Tested logic core**: modules that do not depend on raylib are built and tested on their own.

---

## Known Limitations and Roadmap

Honest notes on what is not finished yet:

- The player's melee attack currently hits to the right only; facing-based attacks are planned.
- Enemy attacks have no wind-up animation yet.
- The Iron Sword and Iron Armor items are defined but not yet obtainable.
- The player behavior profile does not yet track combat distance.
- Dungeon seed is currently fixed; randomized seeds per run are planned.
- Saves are binary and not portable across compilers or platforms.

---

## License

Released under the MIT License. See [LICENSE](LICENSE).