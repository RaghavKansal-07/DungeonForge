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

    input_state.dodge =
        IsKeyPressed(KEY_LEFT_SHIFT) ||
        IsKeyPressed(KEY_RIGHT_SHIFT);
}

InputState Input_GetState(void)
{
    return input_state;
}