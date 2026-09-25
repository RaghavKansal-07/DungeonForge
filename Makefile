CC = gcc

CFLAGS = -Wall -Wextra -std=c17

INCLUDES = -Ithird_party/raylib-6.0_win64_mingw-w64/include -Iinclude

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
      src/save.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) $(LDFLAGS) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET).exe