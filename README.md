# DungeonForge

A procedural 2D roguelike game built entirely in C, featuring procedural dungeon generation, real-time combat, enemy AI, A* pathfinding, binary save/load, and Adaptive Enemy Intelligence.

## Project Status

Feature-complete and undergoing final testing, documentation, and submission preparation.

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
- Boss AI with multiple phases
- Debugging and visualization tools
- Gameplay audio
- Combat and interaction sound effects

## Adaptive Enemy Intelligence

DungeonForge features an event-driven Adaptive Enemy Intelligence system.

The game observes player behavior during a run, including:

- Attack choices
- Dodge direction
- Movement behavior
- Combat distance
- Other gameplay events

The collected information is used to build a behavioral profile of the player. Enemy AI can then adapt its decisions probabilistically based on that profile.

Different enemy archetypes use the behavioral information differently:

- **Hunter** — predicts player dodge direction and adjusts movement.
- **Guardian** — adapts its attack behavior based on the player's attack patterns.
- **Assassin** — uses dodge behavior to influence flanking decisions.
- **Boss** — combines multiple behavioral signals for adaptive decisions.

The system is implemented entirely in C using statistics, probabilities, events, and decision rules rather than machine learning.

## Architecture

```text
Input
  │
  ▼
Game
  │
  ├── World / Dungeon
  │     ├── Tile Map
  │     └── Dungeon Graph
  │
  ├── Entities
  │     ├── Player
  │     └── Enemies
  │
  ├── Combat
  │
  ├── Enemy FSM
  │
  ├── A* Pathfinding
  │
  ├── Events
  │     └── Behavior Analysis
  │
  ├── Adaptive Enemy AI
  │
  ├── Inventory / Items
  │
  ├── Save / Load
  │
  └── Rendering / UI / Audio