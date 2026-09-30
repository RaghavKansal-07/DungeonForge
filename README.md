# DungeonForge

A procedural 2D roguelike game built entirely in C, featuring procedural dungeon generation, real-time combat, enemy AI, A* pathfinding, and Adaptive Enemy Intelligence.

## Project Status

Active development.

Core gameplay systems and Adaptive Enemy Intelligence are implemented.

Phase 17 — UI, audio, and visual polish — is currently in progress.

## Features

- Top-down 2D gameplay
- 8-direction player movement
- Real-time melee and ranged combat
- Procedural dungeon generation
- Tile-based dungeon representation
- Dungeon graph
- Enemy finite-state machine
- Multiple enemy behaviors
- A* pathfinding
- Enemy collision and separation
- Inventory and item system
- Item drops
- Save/load system
- Binary serialization
- Adaptive Enemy Intelligence
- Behavioral player profiling
- Adaptive enemy decision-making
- Boss AI and multiple boss phases
- Debugging tools

## Adaptive Enemy Intelligence

DungeonForge features an event-driven Adaptive Enemy Intelligence system.

The game observes player behavior during a run, including:

- Attack choices
- Dodge direction
- Movement behavior
- Combat distance
- Other gameplay events

The collected information is used to build a behavioral profile of the player. Enemy AI can then adapt its decisions probabilistically based on that profile.

The system is implemented entirely in C using statistics, probabilities, events, and decision rules rather than machine learning.

## Technology

- C17
- raylib
- GCC
- GNU Make

## Build

Build instructions will be added as the project develops.

## Project Structure

```text
DungeonForge/
├── src/          # Game implementation
├── include/      # Header files
├── assets/       # Game assets
├── tests/        # Tests
├── third_party/  # Third-party dependencies
├── saves/        # Runtime save files
├── docs/         # Project documentation
├── Makefile
├── README.md
└── LICENSE