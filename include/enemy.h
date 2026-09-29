#ifndef ENEMY_H
#define ENEMY_H

#include <stdbool.h>

#include "adaptive_ai.h"

#define MAX_ENEMIES 32

typedef enum
{
    ENEMY_STATE_IDLE,
    ENEMY_STATE_CHASE,
    ENEMY_STATE_ATTACK,
    ENEMY_STATE_HURT,
    ENEMY_STATE_DEAD

} EnemyState;

/*
 * Boss-specific states.
 *
 * The boss still uses the normal Enemy system,
 * but these states allow the boss to perform
 * behavior that normal enemies do not have.
 */
typedef enum
{
    BOSS_STATE_IDLE,
    BOSS_STATE_CHASE,
    BOSS_STATE_ATTACK,
    BOSS_STATE_SPECIAL,
    BOSS_STATE_RECOVER,
    BOSS_STATE_ENRAGED

} BossState;

/*
 * Boss health phases.
 *
 * The boss changes behavior as its health decreases.
 */
typedef enum
{
    BOSS_PHASE_ONE,
    BOSS_PHASE_TWO,
    BOSS_PHASE_THREE

} BossPhase;

typedef struct
{
    bool active;

    float x;
    float y;

    float speed;

    int health;
    int max_health;

    float radius;

    EnemyState state;

    /* Attack */
    float attack_cooldown;
    float attack_timer;

    /* Hurt */
    float hurt_timer;

    /* Target / awareness */
    float target_x;
    float target_y;

    float awareness_timer;

    /* Pathfinding */
    float path_timer;

    float waypoint_x;
    float waypoint_y;

    bool path_valid;

    /*
     * Adaptive AI
     *
     * Each enemy has its own archetype and
     * latest adaptive decision.
     */
    AdaptiveAIType adaptive_type;

    AdaptiveDecision adaptive_decision;

    float adaptation_level;

    float adaptive_timer;

    /*
     * -------------------------------------------------
     * Boss AI
     * -------------------------------------------------
     *
     * These fields are only used when the enemy
     * is configured as the boss.
     */
    bool is_boss;

    BossState boss_state;

    BossPhase boss_phase;

    float boss_state_timer;

    float boss_special_timer;

    float boss_recovery_timer;

} Enemy;

void Enemy_Init(void);
void Enemy_Update(void);
void Enemy_Render(void);

Enemy *Enemy_Get(int index);

int Enemy_GetCount(void);

void Enemy_TakeDamage(
    int index,
    int damage
);

/*
 * Restore one enemy from saved game state.
 *
 * This function intentionally keeps the internal
 * enemy array private while allowing the save system
 * to restore a specific enemy slot.
 */
bool Enemy_Restore(
    int index,
    const Enemy *saved_enemy
);

#endif