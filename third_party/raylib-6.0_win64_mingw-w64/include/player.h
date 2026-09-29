#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>

#include "inventory.h"


typedef struct
{
    float x;
    float y;
    float speed;

    int health;

    float attack_cooldown;
    float attack_timer;

    float damage_timer;

    bool dead;

    Inventory inventory;

} Player;


void Player_Init(void);


void Player_SetPosition(
    float x,
    float y
);


void Player_Update(void);
void Player_Render(void);


Player *Player_Get(void);


void Player_TakeDamage(
    int damage
);


bool Player_IsDead(void);


#endif