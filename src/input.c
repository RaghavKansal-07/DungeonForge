#include "input.h"
#include "raylib.h"

static InputState input_state;

void Input_Update(void)
{
    input_state.move_up =
        IsKeyDown(KEY_W);

    input_state.move_down =
        IsKeyDown(KEY_S);

    input_state.move_left =
        IsKeyDown(KEY_A);

    input_state.move_right =
        IsKeyDown(KEY_D);
}

InputState Input_GetState(void)
{
    return input_state;
}