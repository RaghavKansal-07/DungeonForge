#ifndef PATHFINDING_H
#define PATHFINDING_H

#include <stdbool.h>

bool Pathfinding_FindNextStep(
    float start_x,
    float start_y,
    float goal_x,
    float goal_y,
    float radius,
    float *next_x,
    float *next_y
);

#endif