CC = gcc

CFLAGS = -Wall -Wextra -std=c17

INCLUDES = -Iinclude -Ithird_party/raylib-6.0_win64_mingw-w64/include

LDFLAGS = -Lthird_party/raylib-6.0_win64_mingw-w64/lib

LIBS = -lraylib -lopengl32 -lgdi32 -lwinmm

TARGET = dungeonforge

SRC = src/main.c \
      src/game.c \
      src/input.c \
      src/player.c \
      src/tilemap.c \
      src/collision.c \
      src/enemy.c \
      src/enemy_fsm.c \
      src/combat.c \
      src/pathfinding.c \
      src/dungeon.c \
      src/rng.c \
      src/inventory.c \
      src/items.c \
      src/item_drop.c \
      src/events.c \
      src/behavior.c \
      src/adaptive_ai.c \
      src/serialization.c \
      src/save.c \
      src/audio.c \
      src/ui.c \
      src/effects.c \
      src/lighting.c \
      src/minimap.c \
      src/gamecamera.c \
      src/attack_cone.c \
      src/windup.c \
      src/equipment.c


# ---------------------------------------------------------
# Behavior Tests
# ---------------------------------------------------------

TEST_BEHAVIOR = tests/test_behavior.exe

TEST_BEHAVIOR_SRC = tests/test_behavior.c \
                    src/behavior.c \
                    src/events.c


# ---------------------------------------------------------
# Event System Tests
# ---------------------------------------------------------

TEST_EVENTS = tests/test_events.exe

TEST_EVENTS_SRC = tests/test_events.c \
                  src/events.c


# ---------------------------------------------------------
# Adaptive AI Tests
# ---------------------------------------------------------

TEST_ADAPTIVE_AI = tests/test_adaptive_ai.exe

TEST_ADAPTIVE_AI_SRC = tests/test_adaptive_ai.c \
                       src/adaptive_ai.c \
                       src/behavior.c \
                       src/events.c \
                       src/rng.c


# ---------------------------------------------------------
# RNG Tests
# ---------------------------------------------------------

TEST_RNG = tests/test_rng.exe

TEST_RNG_SRC = tests/test_rng.c \
               src/rng.c


# ---------------------------------------------------------
# Serialization Tests
# ---------------------------------------------------------

TEST_SERIALIZATION = tests/test_serialization.exe

TEST_SERIALIZATION_SRC = tests/test_serialization.c \
                         src/serialization.c


# ---------------------------------------------------------
# Pathfinding Tests
# ---------------------------------------------------------

TEST_PATHFINDING = tests/test_pathfinding.exe

TEST_PATHFINDING_SRC = tests/test_pathfinding.c \
                       src/pathfinding.c \
                       src/tilemap.c


# ---------------------------------------------------------
# Dungeon Tests
# ---------------------------------------------------------

TEST_DUNGEON = tests/test_dungeon.exe

TEST_DUNGEON_SRC = tests/test_dungeon.c \
                   src/dungeon.c \
                   src/tilemap.c \
                   src/rng.c


# ---------------------------------------------------------
# Attack Cone Tests
# ---------------------------------------------------------

TEST_ATTACK_CONE = tests/test_attack_cone.exe

TEST_ATTACK_CONE_SRC = tests/test_attack_cone.c \
                       src/attack_cone.c


# ---------------------------------------------------------
# Wind-up Tests
# ---------------------------------------------------------

TEST_WINDUP = tests/test_windup.exe

TEST_WINDUP_SRC = tests/test_windup.c \
                  src/windup.c


# ---------------------------------------------------------
# Balance Tests
# ---------------------------------------------------------

TEST_BALANCE = tests/test_balance.exe

TEST_BALANCE_SRC = tests/test_balance.c


# ---------------------------------------------------------
# Inventory Tests
# ---------------------------------------------------------

TEST_INVENTORY = tests/test_inventory.exe

TEST_INVENTORY_SRC = tests/test_inventory.c \
                     src/inventory.c


# ---------------------------------------------------------
# Equipment Tests
# ---------------------------------------------------------

TEST_EQUIPMENT = tests/test_equipment.exe

TEST_EQUIPMENT_SRC = tests/test_equipment.c \
                     src/equipment.c \
                     src/inventory.c


# ---------------------------------------------------------
# Fight Style Tests
# ---------------------------------------------------------

TEST_FIGHT_STYLE = tests/test_fight_style.exe

TEST_FIGHT_STYLE_SRC = tests/test_fight_style.c \
                       src/behavior.c \
                       src/events.c \
                       src/adaptive_ai.c \
                       src/rng.c


# ---------------------------------------------------------
# Default target
# ---------------------------------------------------------

.PHONY: all test clean

all: $(TARGET)


# ---------------------------------------------------------
# Main Game
# ---------------------------------------------------------

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) $(LDFLAGS) $(LIBS) -o $(TARGET) -lm


# ---------------------------------------------------------
# Behavior Test
# ---------------------------------------------------------

$(TEST_BEHAVIOR): $(TEST_BEHAVIOR_SRC)
	$(CC) $(CFLAGS) -Iinclude $(TEST_BEHAVIOR_SRC) -o $(TEST_BEHAVIOR) -lm


# ---------------------------------------------------------
# Event System Test
# ---------------------------------------------------------

$(TEST_EVENTS): $(TEST_EVENTS_SRC)
	$(CC) $(CFLAGS) -Iinclude $(TEST_EVENTS_SRC) -o $(TEST_EVENTS)


# ---------------------------------------------------------
# Adaptive AI Test
# ---------------------------------------------------------

$(TEST_ADAPTIVE_AI): $(TEST_ADAPTIVE_AI_SRC)
	$(CC) $(CFLAGS) -Iinclude $(TEST_ADAPTIVE_AI_SRC) -o $(TEST_ADAPTIVE_AI) -lm


# ---------------------------------------------------------
# RNG Test
# ---------------------------------------------------------

$(TEST_RNG): $(TEST_RNG_SRC)
	$(CC) $(CFLAGS) -Iinclude $(TEST_RNG_SRC) -o $(TEST_RNG)


# ---------------------------------------------------------
# Serialization Test
# ---------------------------------------------------------

$(TEST_SERIALIZATION): $(TEST_SERIALIZATION_SRC)
	$(CC) $(CFLAGS) -Iinclude $(TEST_SERIALIZATION_SRC) -o $(TEST_SERIALIZATION)


# ---------------------------------------------------------
# Pathfinding Test
# ---------------------------------------------------------

$(TEST_PATHFINDING): $(TEST_PATHFINDING_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_PATHFINDING_SRC) $(LDFLAGS) $(LIBS) -o $(TEST_PATHFINDING) -lm


# ---------------------------------------------------------
# Dungeon Test
# ---------------------------------------------------------

$(TEST_DUNGEON): $(TEST_DUNGEON_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_DUNGEON_SRC) $(LDFLAGS) $(LIBS) -o $(TEST_DUNGEON) -lm


# ---------------------------------------------------------
# Attack Cone Test
# ---------------------------------------------------------

$(TEST_ATTACK_CONE): $(TEST_ATTACK_CONE_SRC)
	$(CC) $(CFLAGS) -Iinclude $(TEST_ATTACK_CONE_SRC) -o $(TEST_ATTACK_CONE) -lm


# ---------------------------------------------------------
# Wind-up Test
# ---------------------------------------------------------

$(TEST_WINDUP): $(TEST_WINDUP_SRC)
	$(CC) $(CFLAGS) -Iinclude $(TEST_WINDUP_SRC) -o $(TEST_WINDUP)


# ---------------------------------------------------------
# Balance Test
# ---------------------------------------------------------

$(TEST_BALANCE): $(TEST_BALANCE_SRC)
	$(CC) $(CFLAGS) -Iinclude $(TEST_BALANCE_SRC) -o $(TEST_BALANCE)


# ---------------------------------------------------------
# Inventory Test
# ---------------------------------------------------------

$(TEST_INVENTORY): $(TEST_INVENTORY_SRC)
	$(CC) $(CFLAGS) -Iinclude $(TEST_INVENTORY_SRC) -o $(TEST_INVENTORY)


# ---------------------------------------------------------
# Equipment Test
# ---------------------------------------------------------

$(TEST_EQUIPMENT): $(TEST_EQUIPMENT_SRC)
	$(CC) $(CFLAGS) -Iinclude $(TEST_EQUIPMENT_SRC) -o $(TEST_EQUIPMENT)


# ---------------------------------------------------------
# Fight Style Test
# ---------------------------------------------------------

$(TEST_FIGHT_STYLE): $(TEST_FIGHT_STYLE_SRC)
	$(CC) $(CFLAGS) -Iinclude $(TEST_FIGHT_STYLE_SRC) -o $(TEST_FIGHT_STYLE) -lm


# ---------------------------------------------------------
# Run All Tests
# ---------------------------------------------------------

test: $(TEST_BEHAVIOR) $(TEST_EVENTS) $(TEST_ADAPTIVE_AI) $(TEST_RNG) $(TEST_SERIALIZATION) $(TEST_PATHFINDING) $(TEST_DUNGEON) $(TEST_ATTACK_CONE) $(TEST_WINDUP) $(TEST_BALANCE) $(TEST_INVENTORY) $(TEST_EQUIPMENT) $(TEST_FIGHT_STYLE)
	./$(TEST_BEHAVIOR)
	./$(TEST_EVENTS)
	./$(TEST_ADAPTIVE_AI)
	./$(TEST_RNG)
	./$(TEST_SERIALIZATION)
	./$(TEST_PATHFINDING)
	./$(TEST_DUNGEON)
	./$(TEST_ATTACK_CONE)
	./$(TEST_WINDUP)
	./$(TEST_BALANCE)
	./$(TEST_INVENTORY)
	./$(TEST_EQUIPMENT)
	./$(TEST_FIGHT_STYLE)


# ---------------------------------------------------------
# Clean
# ---------------------------------------------------------

clean:
	rm -f $(TARGET).exe
	rm -f $(TEST_BEHAVIOR)
	rm -f $(TEST_EVENTS)
	rm -f $(TEST_ADAPTIVE_AI)
	rm -f $(TEST_RNG)
	rm -f $(TEST_SERIALIZATION)
	rm -f $(TEST_PATHFINDING)
	rm -f $(TEST_DUNGEON)
	rm -f $(TEST_ATTACK_CONE)
	rm -f $(TEST_WINDUP)
	rm -f $(TEST_BALANCE)
	rm -f $(TEST_INVENTORY)
	rm -f $(TEST_EQUIPMENT)
	rm -f $(TEST_FIGHT_STYLE)
	rm -f tests/test_serialization.dat