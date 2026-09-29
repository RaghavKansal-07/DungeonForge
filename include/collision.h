#ifndef COLLISION_H
#define COLLISION_H

#include <stdbool.h>

bool Collision_CanMove(
    float x,
    float y,
    float radius
);

bool Collision_PlayerCanMove(
    float x,
    float y,
    float radius
);

bool Collision_EnemyCanMove(
    int enemy_index,
    float x,
    float y,
    float radius
);

bool Collision_HasLineOfSight(
    float x1,
    float y1,
    float x2,
    float y2
);

void Collision_SeparateEnemies(void);

#endif