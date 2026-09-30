#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

typedef struct
{
    bool move_up;
    bool move_down;
    bool move_left;
    bool move_right;

    bool dodge;

} InputState;


void Input_Update(void);

InputState Input_GetState(void);


#endif