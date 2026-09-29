#include "game.h"
#include "raylib.h"

int main(void)
{
    Game_Init();

    while (!WindowShouldClose())
    {
        Game_Update();
        Game_Render();
    }

    Game_Shutdown();

    return 0;
}