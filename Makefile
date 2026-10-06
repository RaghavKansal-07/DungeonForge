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
      src/inventory.c \
      src/items.c \
      src/item_drop.c \
      src/events.c \
      src/behavior.c \
      src/adaptive_ai.c \
      src/serialization.c \
      src/save.c \
      src/audio.c


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
                       src/events.c


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
# Main Game
# ---------------------------------------------------------

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) $(LDFLAGS) $(LIBS) -o $(TARGET)


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
# Run All Tests
# ---------------------------------------------------------

test: $(TEST_BEHAVIOR) $(TEST_EVENTS) $(TEST_ADAPTIVE_AI) $(TEST_SERIALIZATION) $(TEST_PATHFINDING)
	./$(TEST_BEHAVIOR)
	./$(TEST_EVENTS)
	./$(TEST_ADAPTIVE_AI)
	./$(TEST_SERIALIZATION)
	./$(TEST_PATHFINDING)


# ---------------------------------------------------------
# Clean
# ---------------------------------------------------------

clean:
	rm -f $(TARGET).exe
	rm -f $(TEST_BEHAVIOR)
	rm -f $(TEST_EVENTS)
	rm -f $(TEST_ADAPTIVE_AI)
	rm -f $(TEST_SERIALIZATION)
	rm -f $(TEST_PATHFINDING)
	rm -f tests/test_serialization.dat