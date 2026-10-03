#include "enemy_fsm.h"
#include "enemy.h"
#include "player.h"
#include "collision.h"
#include "pathfinding.h"
#include "raylib.h"
#include "audio.h"

#include <math.h>
#include <stdbool.h>

#define ENEMY_DETECTION_RANGE 250.0f
#define ENEMY_LOSE_RANGE      350.0f

#define ENEMY_ATTACK_RANGE    50.0f
#define ENEMY_ATTACK_DAMAGE   10

#define ENEMY_PATH_UPDATE_TIME 0.20f
#define ENEMY_AWARENESS_TIME   1.50f

/*
 * If an enemy cannot make meaningful progress
 * for this amount of time, its current path
 * is considered blocked and A* is recalculated.
 */
#define ENEMY_STUCK_TIME 0.30f

/*
 * Maximum movement distance per collision step.
 */
#define ENEMY_MAX_MOVE_STEP 4.0f


/*
 * ---------------------------------------------------------
 * Boss configuration
 * ---------------------------------------------------------
 */

#define BOSS_DETECTION_RANGE      400.0f
#define BOSS_LOSE_RANGE           550.0f

#define BOSS_ATTACK_RANGE         60.0f
#define BOSS_ATTACK_EXIT_RANGE    72.0f
#define BOSS_ATTACK_DAMAGE        15

#define BOSS_SPECIAL_RANGE        120.0f
#define BOSS_SPECIAL_DAMAGE       20

#define BOSS_SPECIAL_COOLDOWN     5.0f
#define BOSS_RECOVERY_TIME        1.0f

#define BOSS_ENRAGED_SPEED        210.0f

#define BOSS_PHASE_TWO_HEALTH     0.70f
#define BOSS_PHASE_THREE_HEALTH   0.40f

/*
 * Boss-specific navigation tuning.
 *
 * A boss is larger and faster than normal enemies,
 * so tiny movements around a wall corner should not
 * be considered meaningful progress.
 */
#define BOSS_STUCK_TIME            0.20f
#define BOSS_MIN_PROGRESS          0.75f

/*
 * ---------------------------------------------------------
 * Boss dynamic-obstacle yielding
 * ---------------------------------------------------------
 *
 * When the boss is directly blocked by the player,
 * repeatedly trying to move toward the player can
 * create a deadlock in a narrow corridor.
 *
 * The boss therefore briefly backs away to create
 * physical space and allow the player/boss to
 * reposition.
 */
#define BOSS_YIELD_TIME            0.18f
#define BOSS_YIELD_STEP            7.0f
#define BOSS_CONTACT_DISTANCE      58.0f

/*
 * Local boss escape steering.
 *
 * A small "back away" step is not enough near a wall corner.
 * The boss therefore samples several directions and looks ahead
 * before choosing where to create space.
 */
#define BOSS_ESCAPE_PROBE_STEP      4.0f
#define BOSS_ESCAPE_PROBE_COUNT     10
#define BOSS_ESCAPE_ANGLE_STEP      22.5f


typedef enum
{
    ENEMY_CHASE_DIRECT,
    ENEMY_CHASE_PATH

} EnemyChaseMode;


static EnemyChaseMode chase_modes[MAX_ENEMIES];

/*
 * Amount of time each enemy has been unable
 * to make meaningful progress.
 */
static float stuck_timers[MAX_ENEMIES];

/*
 * Last measured distance to the boss movement target.
 *
 * This is used to detect corner vibration where the
 * boss technically moves but does not actually get
 * closer to its destination.
 */
static float boss_previous_target_distance[MAX_ENEMIES];

/*
 * Short-term steering memory.
 *
 * When direct movement is blocked, the enemy remembers the
 * direction it used to go around the obstacle for a short time.
 * This prevents rapid left/right candidate switching, which was
 * causing the visible vibration around corners.
 */
static float steering_direction_x[MAX_ENEMIES];
static float steering_direction_y[MAX_ENEMIES];
static float steering_memory_timer[MAX_ENEMIES];

#define ENEMY_STEERING_MEMORY_TIME 0.20f
#define ENEMY_STEERING_TURN_PENALTY 18.0f


/*
 * Calculate distance between two points.
 */
static float DistanceBetween(
    float x1,
    float y1,
    float x2,
    float y2
)
{
    float dx = x2 - x1;
    float dy = y2 - y1;

    return sqrtf(
        dx * dx +
        dy * dy
    );
}


/*
 * Try one complete movement step.
 *
 * IMPORTANT:
 *
 * We intentionally do NOT perform separate X/Y
 * movement here.
 */
static bool Enemy_TryMove(
    int enemy_index,
    Enemy *enemy,
    float direction_x,
    float direction_y,
    float move_distance
)
{
    float move_x =
        direction_x * move_distance;

    float move_y =
        direction_y * move_distance;

    if (!Collision_EnemyCanMove(
            enemy_index,
            enemy->x + move_x,
            enemy->y + move_y,
            enemy->radius))
    {
        return false;
    }

    enemy->x += move_x;
    enemy->y += move_y;

    return true;
}


/*
 * Move the enemy toward a target.
 *
 * Direct movement is attempted first.
 *
 * If direct movement is blocked, test directions
 * around the desired direction and choose the safe
 * direction that gets closest to the target.
 */
static bool Enemy_MoveToward(
    int enemy_index,
    Enemy *enemy,
    float target_x,
    float target_y
)
{
    float dt =
        GetFrameTime();

    float total_distance =
        enemy->speed * dt;

    if (total_distance <= 0.0f)
        return false;

    if (enemy_index >= 0 &&
        enemy_index < MAX_ENEMIES)
    {
        steering_memory_timer[enemy_index] -=
            dt;

        if (steering_memory_timer[enemy_index] < 0.0f)
            steering_memory_timer[enemy_index] = 0.0f;
    }

    int steps =
        (int)ceilf(
            total_distance /
            ENEMY_MAX_MOVE_STEP
        );

    if (steps < 1)
        steps = 1;

    float step_distance =
        total_distance /
        (float)steps;

    bool moved_anything = false;

    for (int step = 0;
         step < steps;
         step++)
    {
        float dx =
            target_x - enemy->x;

        float dy =
            target_y - enemy->y;

        float distance =
            sqrtf(
                dx * dx +
                dy * dy
            );

        if (distance <= 0.001f)
            break;

        float direction_x =
            dx / distance;

        float direction_y =
            dy / distance;

        /*
         * -------------------------------------------------
         * 1. Always try direct movement first.
         * -------------------------------------------------
         *
         * Steering memory is only used when direct movement
         * is blocked. This keeps normal open-room chasing
         * completely direct.
         */
        if (Collision_EnemyCanMove(
                enemy_index,
                enemy->x +
                    direction_x *
                    step_distance,
                enemy->y +
                    direction_y *
                    step_distance,
                enemy->radius))
        {
            if (Enemy_TryMove(
                    enemy_index,
                    enemy,
                    direction_x,
                    direction_y,
                    step_distance))
            {
                moved_anything = true;

                if (enemy_index >= 0 &&
                    enemy_index < MAX_ENEMIES)
                {
                    steering_direction_x[enemy_index] =
                        direction_x;

                    steering_direction_y[enemy_index] =
                        direction_y;

                    steering_memory_timer[enemy_index] =
                        0.0f;
                }

                continue;
            }
        }

        /*
         * -------------------------------------------------
         * 2. Direct movement is blocked.
         * -------------------------------------------------
         *
         * Test steering directions around the desired vector.
         *
         * Previously the candidate with the smallest remaining
         * distance was always selected. Near a corner, two
         * candidates could be almost equally good, causing:
         *
         *     LEFT -> RIGHT -> LEFT -> RIGHT
         *
         * every few frames.
         *
         * The previous steering direction now receives a bias,
         * so the enemy commits to one side briefly instead of
         * changing sides every frame.
         */
        static const float angles[] =
        {
             15.0f,
            -15.0f,
             30.0f,
            -30.0f,
             45.0f,
            -45.0f,
             60.0f,
            -60.0f,
             75.0f,
            -75.0f,
             90.0f,
            -90.0f,
            105.0f,
           -105.0f,
            120.0f,
           -120.0f,
            135.0f,
           -135.0f,
            150.0f,
           -150.0f,
            165.0f,
           -165.0f,
            180.0f
        };

        const int angle_count =
            sizeof(angles) /
            sizeof(angles[0]);

        bool found_candidate = false;

        float best_direction_x =
            0.0f;

        float best_direction_y =
            0.0f;

        float best_score =
            INFINITY;

        for (int i = 0;
             i < angle_count;
             i++)
        {
            float angle =
                angles[i] *
                (PI / 180.0f);

            float cos_angle =
                cosf(angle);

            float sin_angle =
                sinf(angle);

            float candidate_x =
                direction_x * cos_angle -
                direction_y * sin_angle;

            float candidate_y =
                direction_x * sin_angle +
                direction_y * cos_angle;

            float candidate_position_x =
                enemy->x +
                candidate_x *
                step_distance;

            float candidate_position_y =
                enemy->y +
                candidate_y *
                step_distance;

            if (!Collision_EnemyCanMove(
                    enemy_index,
                    candidate_position_x,
                    candidate_position_y,
                    enemy->radius))
            {
                continue;
            }

            float remaining_dx =
                target_x -
                candidate_position_x;

            float remaining_dy =
                target_y -
                candidate_position_y;

            float remaining_distance =
                sqrtf(
                    remaining_dx *
                        remaining_dx +
                    remaining_dy *
                        remaining_dy
                );

            float score =
                remaining_distance;

            if (enemy_index >= 0 &&
                enemy_index < MAX_ENEMIES &&
                steering_memory_timer[enemy_index] > 0.0f)
            {
                float alignment =
                    candidate_x *
                        steering_direction_x[enemy_index] +
                    candidate_y *
                        steering_direction_y[enemy_index];

                float turn_cost =
                    (1.0f - alignment) *
                    ENEMY_STEERING_TURN_PENALTY;

                score += turn_cost;
            }

            if (!found_candidate ||
                score < best_score)
            {
                found_candidate = true;

                best_score =
                    score;

                best_direction_x =
                    candidate_x;

                best_direction_y =
                    candidate_y;
            }
        }

        if (!found_candidate)
            break;

        if (Enemy_TryMove(
                enemy_index,
                enemy,
                best_direction_x,
                best_direction_y,
                step_distance))
        {
            moved_anything = true;

            if (enemy_index >= 0 &&
                enemy_index < MAX_ENEMIES)
            {
                steering_direction_x[enemy_index] =
                    best_direction_x;

                steering_direction_y[enemy_index] =
                    best_direction_y;

                steering_memory_timer[enemy_index] =
                    ENEMY_STEERING_MEMORY_TIME;
            }
        }
        else
        {
            break;
        }
    }

    return moved_anything;
}


/*
 * ---------------------------------------------------------
 * Boss direct movement
 * ---------------------------------------------------------
 *
 * The boss deliberately does NOT use Enemy_MoveToward()
 * for its primary movement.
 *
 * Enemy_MoveToward() contains local steering. That is useful
 * for normal enemies, but a large boss can oscillate between
 * two locally valid directions when it is close to a wall.
 *
 * The boss therefore uses only the direct vector here.
 * If that vector is blocked, Boss_MoveTowardPlayer() switches
 * to A* instead of trying left/right steering every frame.
 */
static bool Boss_MoveDirectToward(
    int enemy_index,
    Enemy *boss,
    float target_x,
    float target_y
)
{
    float dt =
        GetFrameTime();

    float total_distance =
        boss->speed * dt;

    if (total_distance <= 0.0f)
        return false;

    int steps =
        (int)ceilf(
            total_distance /
            ENEMY_MAX_MOVE_STEP
        );

    if (steps < 1)
        steps = 1;

    float step_distance =
        total_distance /
        (float)steps;

    bool moved_anything = false;

    for (int step = 0;
         step < steps;
         step++)
    {
        float dx =
            target_x - boss->x;

        float dy =
            target_y - boss->y;

        float distance =
            sqrtf(
                dx * dx +
                dy * dy
            );

        if (distance <= 0.001f)
            break;

        float direction_x =
            dx / distance;

        float direction_y =
            dy / distance;

        if (!Enemy_TryMove(
                enemy_index,
                boss,
                direction_x,
                direction_y,
                step_distance))
        {
            break;
        }

        moved_anything = true;
    }

    return moved_anything;
}

/*
 * Calculate an A* waypoint.
 */
static bool Enemy_UpdatePath(
    Enemy *enemy,
    float goal_x,
    float goal_y
)
{
    float next_x;
    float next_y;

    bool found =
        Pathfinding_FindNextStep(
            enemy->x,
            enemy->y,
            goal_x,
            goal_y,
            enemy->radius,
            &next_x,
            &next_y
        );

    if (!found)
    {
        enemy->path_valid = false;
        return false;
    }

    enemy->waypoint_x =
        next_x;

    enemy->waypoint_y =
        next_y;

    enemy->path_valid =
        true;

    return true;
}


/*
 * Reset navigation state for one enemy.
 */
static void Enemy_ResetNavigation(
    int index,
    Enemy *enemy
)
{
    enemy->path_valid = false;
    enemy->path_timer = 0.0f;

    stuck_timers[index] = 0.0f;

    chase_modes[index] =
        ENEMY_CHASE_DIRECT;

    boss_previous_target_distance[index] =
        0.0f;

    steering_direction_x[index] =
        0.0f;

    steering_direction_y[index] =
        0.0f;

    steering_memory_timer[index] =
        0.0f;
}


/*
 * Update how long the enemy has been
 * unable to make progress.
 */
static void Enemy_UpdateStuckTimer(
    int index,
    bool moved
)
{
    float dt =
        GetFrameTime();

    if (moved)
    {
        stuck_timers[index] -=
            dt * 2.0f;

        if (stuck_timers[index] < 0.0f)
            stuck_timers[index] = 0.0f;

        return;
    }

    stuck_timers[index] += dt;
}


/*
 * Check whether the enemy has been stuck.
 */
static bool Enemy_IsStuck(
    int index
)
{
    return stuck_timers[index] >=
           ENEMY_STUCK_TIME;
}


/*
 * ---------------------------------------------------------
 * Adaptive AI helpers
 * ---------------------------------------------------------
 */

static void Enemy_GetPredictedTarget(
    Enemy *enemy,
    Player *player,
    float *target_x,
    float *target_y
)
{
    *target_x = player->x;
    *target_y = player->y;

    const float prediction_distance =
        45.0f;

    switch (enemy->adaptive_decision)
    {
        case ADAPTIVE_DECISION_PREDICT_LEFT:
            *target_x -= prediction_distance;
            break;

        case ADAPTIVE_DECISION_PREDICT_RIGHT:
            *target_x += prediction_distance;
            break;

        case ADAPTIVE_DECISION_PREDICT_UP:
            *target_y -= prediction_distance;
            break;

        case ADAPTIVE_DECISION_PREDICT_DOWN:
            *target_y += prediction_distance;
            break;

        default:
            break;
    }
}


static void Enemy_GetFlankTarget(
    Enemy *enemy,
    Player *player,
    float *target_x,
    float *target_y
)
{
    float dx =
        player->x - enemy->x;

    float dy =
        player->y - enemy->y;

    float distance =
        sqrtf(
            dx * dx +
            dy * dy
        );

    if (distance <= 0.001f)
    {
        *target_x = player->x;
        *target_y = player->y;
        return;
    }

    dx /= distance;
    dy /= distance;

    float perpendicular_x =
        -dy;

    float perpendicular_y =
        dx;

    const float flank_distance =
        70.0f;

    if (enemy->adaptive_decision ==
        ADAPTIVE_DECISION_FLANK_RIGHT)
    {
        perpendicular_x =
            -perpendicular_x;

        perpendicular_y =
            -perpendicular_y;
    }

    *target_x =
        player->x +
        perpendicular_x *
        flank_distance;

    *target_y =
        player->y +
        perpendicular_y *
        flank_distance;
}


static bool Enemy_GetAdaptiveTarget(
    Enemy *enemy,
    Player *player,
    float *target_x,
    float *target_y
)
{
    if (enemy->adaptation_level <= 0.0f)
        return false;

    switch (enemy->adaptive_decision)
    {
        case ADAPTIVE_DECISION_PREDICT_LEFT:
        case ADAPTIVE_DECISION_PREDICT_RIGHT:
        case ADAPTIVE_DECISION_PREDICT_UP:
        case ADAPTIVE_DECISION_PREDICT_DOWN:
        {
            Enemy_GetPredictedTarget(
                enemy,
                player,
                target_x,
                target_y
            );

            return true;
        }

        case ADAPTIVE_DECISION_FLANK_LEFT:
        case ADAPTIVE_DECISION_FLANK_RIGHT:
        {
            Enemy_GetFlankTarget(
                enemy,
                player,
                target_x,
                target_y
            );

            return true;
        }

        case ADAPTIVE_DECISION_CLOSE_DISTANCE:
        {
            float dx =
                player->x - enemy->x;

            float dy =
                player->y - enemy->y;

            float distance =
                sqrtf(
                    dx * dx +
                    dy * dy
                );

            if (distance <= 0.001f)
            {
                *target_x = player->x;
                *target_y = player->y;
                return true;
            }

            dx /= distance;
            dy /= distance;

            const float desired_distance =
                BOSS_ATTACK_RANGE * 0.75f;

            *target_x =
                player->x -
                dx * desired_distance;

            *target_y =
                player->y -
                dy * desired_distance;

            return true;
        }

        default:
            return false;
    }
}


static bool Enemy_ShouldAdaptAttack(
    const Enemy *enemy
)
{
    if (enemy->adaptation_level <= 0.0f)
        return false;

    if (enemy->adaptive_decision ==
            ADAPTIVE_DECISION_ATTACK_AGGRESSIVE ||
        enemy->adaptive_decision ==
            ADAPTIVE_DECISION_ATTACK_DEFENSIVE)
    {
        return true;
    }

    return false;
}


/*
 * ---------------------------------------------------------
 * Boss AI
 * ---------------------------------------------------------
 */

static BossPhase Boss_GetPhase(
    const Enemy *boss
)
{
    if (boss->max_health <= 0)
        return BOSS_PHASE_ONE;

    float health_ratio =
        (float)boss->health /
        (float)boss->max_health;

    if (health_ratio <=
        BOSS_PHASE_THREE_HEALTH)
    {
        return BOSS_PHASE_THREE;
    }

    if (health_ratio <=
        BOSS_PHASE_TWO_HEALTH)
    {
        return BOSS_PHASE_TWO;
    }

    return BOSS_PHASE_ONE;
}


static void Boss_UpdatePhase(
    Enemy *boss
)
{
    BossPhase new_phase =
        Boss_GetPhase(boss);

    if (new_phase !=
        boss->boss_phase)
    {
        boss->boss_phase =
            new_phase;

        Audio_PlayBossPhase();    

        boss->boss_state_timer =
            0.0f;

        boss->boss_special_timer =
            0.0f;

        /*
         * Force fresh navigation after a phase
         * transition.
         */
        boss->path_valid =
            false;

        boss->path_timer =
            0.0f;
    }

    if (boss->boss_phase ==
        BOSS_PHASE_THREE)
    {
        boss->speed =
            BOSS_ENRAGED_SPEED;
    }
    else if (boss->boss_phase ==
             BOSS_PHASE_TWO)
    {
        boss->speed =
            175.0f;
    }
    else
    {
        boss->speed =
            150.0f;
    }
}


static bool Boss_CanUseSpecial(
    const Enemy *boss
)
{
    return boss->boss_special_timer <=
           0.0f;
}


static void Boss_PerformSpecial(
    Enemy *boss,
    Player *player
)
{
    float distance =
        DistanceBetween(
            boss->x,
            boss->y,
            player->x,
            player->y
        );

    if (distance <=
        BOSS_SPECIAL_RANGE &&
        Collision_HasLineOfSight(
            boss->x,
            boss->y,
            player->x,
            player->y))
    {
        Player_TakeDamage(
            BOSS_SPECIAL_DAMAGE
        );
        Audio_PlayBossAttack();
    }

    switch (boss->boss_phase)
    {
        case BOSS_PHASE_ONE:
            boss->boss_special_timer =
                BOSS_SPECIAL_COOLDOWN;
            break;

        case BOSS_PHASE_TWO:
            boss->boss_special_timer =
                BOSS_SPECIAL_COOLDOWN *
                0.80f;
            break;

        case BOSS_PHASE_THREE:
            boss->boss_special_timer =
                BOSS_SPECIAL_COOLDOWN *
                0.60f;
            break;
    }

    boss->boss_recovery_timer =
        BOSS_RECOVERY_TIME;

    boss->boss_state =
        BOSS_STATE_RECOVER;
}


static void Boss_GetMovementTarget(
    Enemy *boss,
    Player *player,
    float *target_x,
    float *target_y
)
{
    *target_x =
        player->x;

    *target_y =
        player->y;

    Enemy_GetAdaptiveTarget(
        boss,
        player,
        target_x,
        target_y
    );
}


/*
 * ---------------------------------------------------------
 * Boss yield / reposition
 * ---------------------------------------------------------
 *
 * The player is a dynamic obstacle, not a wall.
 *
 * If the boss has reached a point where it cannot
 * make meaningful progress toward the player, it
 * briefly moves away from the player.
 *
 * This is especially important in narrow corridors:
 *
 *        PLAYER
 *           |
 *           |
 *          BOSS
 *
 * Without this behavior both entities can continually
 * attempt to occupy the same forward space.
 */
static bool Boss_TryYield(
    int index,
    Enemy *boss,
    Player *player
)
{
    /*
     * ---------------------------------------------------------
     * Local escape steering
     * ---------------------------------------------------------
     *
     * The old implementation tried only:
     *
     *     away, +/-20, +/-40, +/-60 degrees
     *
     * and accepted the first direction that was immediately
     * possible.
     *
     * That is not enough for a large boss near a wall corner.
     * The first "safe" direction can still lead directly into
     * another wall a few pixels later.
     *
     * Instead, evaluate a full ring of directions and inspect
     * how much usable space exists in each direction.
     *
     * This is LOCAL steering only. Long-range navigation remains
     * A*'s responsibility.
     */

    float dx =
        boss->x - player->x;

    float dy =
        boss->y - player->y;

    float distance =
        sqrtf(
            dx * dx +
            dy * dy
        );

    if (distance <= 0.001f)
    {
        /*
         * Exact overlap is rare. Use a deterministic direction
         * rather than allowing an undefined normalized vector.
         */
        dx = 0.0f;
        dy = 1.0f;
        distance = 1.0f;
    }

    /*
     * Base direction points directly away from the player.
     */
    dx /= distance;
    dy /= distance;

    /*
     * Candidate directions are spaced around the full circle.
     *
     * Starting from "away from player" means that equal-space
     * candidates still naturally prefer separation.
     */
    const int angle_count = 16;

    bool found_candidate = false;

    float best_direction_x =
        0.0f;

    float best_direction_y =
        0.0f;

    float best_score =
        -INFINITY;

    float best_clearance =
        0.0f;

    float best_separation_gain =
        -INFINITY;

    for (int i = 0;
         i < angle_count;
         i++)
    {
        float angle =
            ((float)i * BOSS_ESCAPE_ANGLE_STEP) *
            (PI / 180.0f);

        float cos_angle =
            cosf(angle);

        float sin_angle =
            sinf(angle);

        float direction_x =
            dx * cos_angle -
            dy * sin_angle;

        float direction_y =
            dx * sin_angle +
            dy * cos_angle;

        /*
         * First make sure the boss can actually take the
         * requested yield step.
         */
        if (!Collision_EnemyCanMove(
                index,
                boss->x +
                    direction_x *
                    BOSS_YIELD_STEP,
                boss->y +
                    direction_y *
                    BOSS_YIELD_STEP,
                boss->radius))
        {
            continue;
        }

        /*
         * -----------------------------------------------------
         * Probe usable space.
         * -----------------------------------------------------
         *
         * We do not move the boss during this test.
         *
         * A candidate that is technically open for 3 pixels but
         * immediately ends at a wall should lose to a direction
         * with a real escape route.
         */
        float clearance =
            0.0f;

        for (int probe = 1;
             probe <= BOSS_ESCAPE_PROBE_COUNT;
             probe++)
        {
            float probe_distance =
                (float)probe *
                BOSS_ESCAPE_PROBE_STEP;

            float probe_x =
                boss->x +
                direction_x *
                probe_distance;

            float probe_y =
                boss->y +
                direction_y *
                probe_distance;

            if (!Collision_EnemyCanMove(
                    index,
                    probe_x,
                    probe_y,
                    boss->radius))
            {
                break;
            }

            clearance =
                probe_distance;
        }

        /*
         * The immediate movement was already verified, so
         * clearance should normally be at least the yield step.
         */
        if (clearance <= 0.0f)
            continue;

        /*
         * How much farther from the player would this direction
         * move the boss after the probe distance?
         */
        float probe_x =
            boss->x +
            direction_x *
            clearance;

        float probe_y =
            boss->y +
            direction_y *
            clearance;

        float final_distance =
            DistanceBetween(
                probe_x,
                probe_y,
                player->x,
                player->y
            );

        float separation_gain =
            final_distance -
            distance;

        /*
         * A space-making move must not intentionally move
         * toward the player.  The previous scoring system gave
         * environmental clearance such a large weight that a
         * wide but player-facing direction could occasionally
         * win.  That is exactly the wrong behavior when the
         * boss is trying to escape contact.
         */
        if (separation_gain < -0.5f)
            continue;

        /*
         * Alignment with the direct "away from player" vector.
         *
         * +1 = directly away
         *  0 = sideways
         * -1 = toward the player
         */
        float away_alignment =
            direction_x * dx +
            direction_y * dy;

        /*
         * Score:
         *
         * 1. Clearance is the strongest factor.
         *    This is what prevents wall-corner trapping.
         *
         * 2. Separation is next.
         *    The boss should still create distance.
         *
         * 3. Away alignment provides a small preference for
         *    retreating rather than circling.
         *
         * 4. Existing steering memory gets only a small bonus.
         *    It helps avoid unnecessary direction changes but
         *    cannot override the environment.
         */
        float score =
            clearance * 5.0f +
            separation_gain * 10.0f +
            away_alignment * 18.0f;

        if (index >= 0 &&
            index < MAX_ENEMIES &&
            steering_memory_timer[index] > 0.0f)
        {
            float steering_alignment =
                direction_x *
                    steering_direction_x[index] +
                direction_y *
                    steering_direction_y[index];

            score +=
                steering_alignment *
                4.0f;
        }

        /*
         * Deterministic tie-breaking:
         *
         * If two directions are effectively equal, prefer the
         * one with more actual separation.
         */
        if (!found_candidate ||
            score > best_score ||
            (fabsf(score - best_score) < 0.001f &&
             separation_gain > best_separation_gain))
        {
            found_candidate = true;

            best_score =
                score;

            best_direction_x =
                direction_x;

            best_direction_y =
                direction_y;

            best_clearance =
                clearance;

            best_separation_gain =
                separation_gain;
        }
    }

    if (!found_candidate)
        return false;

    /*
     * Take only the small movement step now.
     *
     * We deliberately do NOT move the entire probed clearance.
     * The boss should smoothly create space rather than teleport
     * or make a large local jump.
     */
    if (!Enemy_TryMove(
            index,
            boss,
            best_direction_x,
            best_direction_y,
            BOSS_YIELD_STEP))
    {
        return false;
    }

    if (index >= 0 &&
        index < MAX_ENEMIES)
    {
        steering_direction_x[index] =
            best_direction_x;

        steering_direction_y[index] =
            best_direction_y;

        steering_memory_timer[index] =
            ENEMY_STEERING_MEMORY_TIME;
    }

    /*
     * Keep the variable meaningful for debugging/maintenance:
     * the selected clearance is intentionally calculated but only
     * used for direction selection, not for movement distance.
     */
    (void)best_clearance;

    return true;
}


/*
 * ---------------------------------------------------------
 * Boss local obstacle steering
 * ---------------------------------------------------------
 *
 * This is deliberately different from Boss_TryYield().
 *
 * Boss_TryYield() is for a close player contact situation and
 * chooses a direction that creates separation from the player.
 *
 * This function is for a navigation blockage:
 *
 *     boss -> waypoint/player
 *             X wall corner
 *
 * In that situation the boss may be nowhere near the player,
 * so moving away from the player is the wrong response.  The
 * boss needs to slide around the obstacle and continue toward
 * its navigation target.
 *
 * We therefore score directions using:
 *
 *  1. direction toward the current navigation target
 *  2. usable clearance in front of the boss
 *  3. actual progress toward the target
 *  4. a small steering-memory term to avoid rapid left/right
 *     oscillation
 *
 * Collision_EnemyCanMove() is used for every probe, so this
 * steering cannot move through walls, the player, or another
 * living enemy.
 */
static bool Boss_TryLocalObstacleSteer(
    int index,
    Enemy *boss,
    float target_x,
    float target_y
)
{
    float to_target_x =
        target_x - boss->x;

    float to_target_y =
        target_y - boss->y;

    float target_distance =
        sqrtf(
            to_target_x * to_target_x +
            to_target_y * to_target_y
        );

    if (target_distance <= 0.001f)
        return false;

    to_target_x /= target_distance;
    to_target_y /= target_distance;

    const int angle_count = 16;

    bool found_candidate = false;

    float best_direction_x = 0.0f;
    float best_direction_y = 0.0f;
    float best_score = -INFINITY;
    float best_progress = -INFINITY;
    float best_clearance = 0.0f;

    for (int i = 0;
         i < angle_count;
         i++)
    {
        float angle =
            ((float)i * BOSS_ESCAPE_ANGLE_STEP) *
            (PI / 180.0f);

        float cos_angle = cosf(angle);
        float sin_angle = sinf(angle);

        float direction_x =
            to_target_x * cos_angle -
            to_target_y * sin_angle;

        float direction_y =
            to_target_x * sin_angle +
            to_target_y * cos_angle;

        if (!Collision_EnemyCanMove(
                index,
                boss->x +
                    direction_x * BOSS_YIELD_STEP,
                boss->y +
                    direction_y * BOSS_YIELD_STEP,
                boss->radius))
        {
            continue;
        }

        float clearance = 0.0f;

        for (int probe = 1;
             probe <= BOSS_ESCAPE_PROBE_COUNT;
             probe++)
        {
            float probe_distance =
                (float)probe *
                BOSS_ESCAPE_PROBE_STEP;

            float probe_x =
                boss->x +
                direction_x * probe_distance;

            float probe_y =
                boss->y +
                direction_y * probe_distance;

            if (!Collision_EnemyCanMove(
                    index,
                    probe_x,
                    probe_y,
                    boss->radius))
            {
                break;
            }

            clearance = probe_distance;
        }

        if (clearance <= 0.0f)
            continue;

        float probe_x =
            boss->x +
            direction_x * clearance;

        float probe_y =
            boss->y +
            direction_y * clearance;

        float final_distance =
            DistanceBetween(
                probe_x,
                probe_y,
                target_x,
                target_y
            );

        float progress =
            target_distance -
            final_distance;

        float target_alignment =
            direction_x * to_target_x +
            direction_y * to_target_y;

        float score =
            target_alignment * 55.0f +
            progress * 8.0f +
            clearance * 2.5f;

        if (index >= 0 &&
            index < MAX_ENEMIES &&
            steering_memory_timer[index] > 0.0f)
        {
            float steering_alignment =
                direction_x *
                    steering_direction_x[index] +
                direction_y *
                    steering_direction_y[index];

            score +=
                steering_alignment *
                3.0f;
        }

        if (!found_candidate ||
            score > best_score ||
            (fabsf(score - best_score) < 0.001f &&
             progress > best_progress))
        {
            found_candidate = true;
            best_score = score;
            best_progress = progress;
            best_clearance = clearance;
            best_direction_x = direction_x;
            best_direction_y = direction_y;
        }
    }

    if (!found_candidate)
        return false;

    if (!Enemy_TryMove(
            index,
            boss,
            best_direction_x,
            best_direction_y,
            BOSS_YIELD_STEP))
    {
        return false;
    }

    if (index >= 0 &&
        index < MAX_ENEMIES)
    {
        steering_direction_x[index] =
            best_direction_x;

        steering_direction_y[index] =
            best_direction_y;

        steering_memory_timer[index] =
            ENEMY_STEERING_MEMORY_TIME;
    }

    (void)best_clearance;

    return true;
}


/*
 * Boss movement.
 *
 * Important changes:
 *
 * 1. Navigation is reset cleanly whenever a fresh
 *    pursuit begins.
 *
 * 2. Direct movement is used only while the boss
 *    is actually in DIRECT chase mode.
 *
 * 3. Once the boss switches to PATH mode, LOS no
 *    longer immediately forces it back into DIRECT.
 *
 * 4. This allows A* to actually take control after
 *    a direct-chase blockage.
 *
 * 5. If the blockage is a dynamic player obstacle,
 *    the boss can briefly yield and create space.
 *
 * 6. Direct movement still handles normal open-room
 *    chasing.
 *
 * 7. The boss no longer uses local left/right steering for
 *    direct pursuit. A blocked direct move hands control to A*.
 */
static void Boss_MoveTowardPlayer(
    int index,
    Enemy *boss,
    Player *player
)
{
    /*
     * -------------------------------------------------
     * TEMPORARY YIELD
     * -------------------------------------------------
     *
     * If the previous frame detected a dynamic
     * blockage, allow the boss to back away briefly.
     */
    if (boss->boss_state_timer > 0.0f)
    {
        if (Boss_TryYield(
                index,
                boss,
                player))
        {
            return;
        }

        /*
         * If even backing away is impossible,
         * stop yielding and continue normal
         * navigation.
         */
        boss->boss_state_timer =
            0.0f;
    }

    float target_x;
    float target_y;

    Boss_GetMovementTarget(
        boss,
        player,
        &target_x,
        &target_y
    );

    float target_distance_before =
        DistanceBetween(
            boss->x,
            boss->y,
            target_x,
            target_y
        );

    bool has_line_of_sight =
        Collision_HasLineOfSight(
            boss->x,
            boss->y,
            player->x,
            player->y
        );

    /*
     * -------------------------------------------------
     * DIRECT MOVEMENT
     * -------------------------------------------------
     *
     * IMPORTANT:
     *
     * LOS alone is no longer enough to force direct
     * chase. The boss uses a straight physical move only.
     * If that move is blocked, A* takes over instead of
     * local steering oscillating around the obstacle.
     *
     * If the boss has already switched to PATH mode,
     * it stays in PATH mode until the current
     * navigation attempt has been resolved.
     */
    if (has_line_of_sight &&
        chase_modes[index] ==
            ENEMY_CHASE_DIRECT)
    {
        bool moved =
            Boss_MoveDirectToward(
                index,
                boss,
                target_x,
                target_y
            );

        float target_distance_after =
            DistanceBetween(
                boss->x,
                boss->y,
                target_x,
                target_y
            );

        /*
         * A movement that does not reduce the target
         * distance meaningfully is considered blocked.
         *
         * This catches wall-corner vibration and
         * dynamic-player blocking.
         */
        bool meaningful_progress =
            target_distance_before -
            target_distance_after >=
            BOSS_MIN_PROGRESS;

        if (!moved ||
            !meaningful_progress)
        {
            stuck_timers[index] +=
                GetFrameTime();

            /*
             * The boss can be blocked by a WALL CORNER even when
             * the player is far away.  Do not wait for the stuck
             * timer in that situation.  Locally slide around the
             * obstacle toward the current navigation target, then
             * let A* rebuild the route from the new position.
             */
            if (Boss_TryLocalObstacleSteer(
                    index,
                    boss,
                    target_x,
                    target_y))
            {
                boss->path_valid =
                    false;

                boss->path_timer =
                    0.0f;

                chase_modes[index] =
                    ENEMY_CHASE_PATH;

                stuck_timers[index] =
                    0.0f;

                return;
            }

            /*
             * If the player is physically close and the direct
             * chase step is blocked, treat the player as a
             * dynamic obstacle immediately.  Do not wait for the
             * general stuck timer to expire: the boss should make
             * room as soon as contact prevents forward motion.
             */
            if (DistanceBetween(
                    boss->x,
                    boss->y,
                    player->x,
                    player->y) <=
                    BOSS_CONTACT_DISTANCE + 6.0f &&
                Boss_TryYield(
                    index,
                    boss,
                    player))
            {
                boss->boss_state_timer =
                    BOSS_YIELD_TIME;

                stuck_timers[index] =
                    0.0f;

                return;
            }
        }
        else
        {
            stuck_timers[index] -=
                GetFrameTime() * 2.0f;

            if (stuck_timers[index] < 0.0f)
                stuck_timers[index] = 0.0f;
        }

        /*
         * If the adaptive target is causing the
         * obstruction, try the actual player position.
         */
        if ((!moved ||
             !meaningful_progress) &&
            (target_x != player->x ||
             target_y != player->y))
        {
            bool fallback_moved =
                Boss_MoveDirectToward(
                    index,
                    boss,
                    player->x,
                    player->y
                );

            if (fallback_moved)
            {
                float fallback_distance =
                    DistanceBetween(
                        boss->x,
                        boss->y,
                        player->x,
                        player->y
                    );

                if (target_distance_before -
                    fallback_distance >=
                    BOSS_MIN_PROGRESS)
                {
                    stuck_timers[index] -=
                        GetFrameTime() * 2.0f;

                    if (stuck_timers[index] < 0.0f)
                        stuck_timers[index] = 0.0f;
                }
            }
        }

        /*
         * Switch to A* when the boss has reached
         * a corner, obstacle, or dynamic blockage.
         */
        if (stuck_timers[index] >=
            BOSS_STUCK_TIME)
        {
            boss->path_valid =
                false;

            boss->path_timer =
                0.0f;

            /*
             * First try yielding to the player.
             *
             * This gives a trapped player room to
             * retreat instead of producing a permanent
             * face-to-face deadlock.
             */
            if (DistanceBetween(
                    boss->x,
                    boss->y,
                    player->x,
                    player->y) <=
                    BOSS_CONTACT_DISTANCE + 8.0f &&
                Boss_TryYield(
                    index,
                    boss,
                    player))
            {
                boss->boss_state_timer =
                    BOSS_YIELD_TIME;

                stuck_timers[index] =
                    0.0f;

                return;
            }

            stuck_timers[index] =
                0.0f;

            chase_modes[index] =
                ENEMY_CHASE_PATH;
        }

        return;
    }


    /*
     * -------------------------------------------------
     * PATH CHASE
     * -------------------------------------------------
     *
     * No direct route is currently being trusted.
     *
     * IMPORTANT:
     *
     * This branch is allowed to execute even when
     * line of sight is TRUE.
     *
     * This is the key fix for the previous bug.
     */
    chase_modes[index] =
        ENEMY_CHASE_PATH;

    if (boss->path_timer <= 0.0f ||
        !boss->path_valid)
    {
        /*
         * First try the adaptive destination.
         */
        bool path_found =
            Enemy_UpdatePath(
                boss,
                target_x,
                target_y
            );

        /*
         * If the adaptive target cannot be reached,
         * route directly to the player.
         */
        if (!path_found &&
            (target_x != player->x ||
             target_y != player->y))
        {
            path_found =
                Enemy_UpdatePath(
                    boss,
                    player->x,
                    player->y
                );
        }

        boss->path_timer =
            ENEMY_PATH_UPDATE_TIME;

        /*
         * If no route was found, allow the local
         * collision-aware fallback to try.
         */
        if (!path_found)
        {
            boss->path_valid =
                false;
        }
    }

    if (boss->path_valid)
    {
        float waypoint_distance_before =
            DistanceBetween(
                boss->x,
                boss->y,
                boss->waypoint_x,
                boss->waypoint_y
            );

        bool moved =
            Boss_MoveDirectToward(
                index,
                boss,
                boss->waypoint_x,
                boss->waypoint_y
            );

        float waypoint_distance_after =
            DistanceBetween(
                boss->x,
                boss->y,
                boss->waypoint_x,
                boss->waypoint_y
            );

        bool meaningful_progress =
            waypoint_distance_before -
            waypoint_distance_after >=
            BOSS_MIN_PROGRESS;

        if (!moved ||
            !meaningful_progress)
        {
            stuck_timers[index] +=
                GetFrameTime();

            /*
             * The waypoint can be geometrically valid for A* but
             * still be a poor immediate movement direction for a
             * large boss at a tile corner.  Slide around the
             * obstruction immediately instead of repeatedly
             * requesting the same blocked waypoint.
             */
            if (Boss_TryLocalObstacleSteer(
                    index,
                    boss,
                    boss->waypoint_x,
                    boss->waypoint_y))
            {
                boss->path_valid =
                    false;

                boss->path_timer =
                    0.0f;

                stuck_timers[index] =
                    0.0f;

                return;
            }
        }
        else
        {
            stuck_timers[index] -=
                GetFrameTime() * 2.0f;

            if (stuck_timers[index] < 0.0f)
                stuck_timers[index] = 0.0f;
        }

        if (waypoint_distance_after <
            7.0f)
        {
            boss->path_timer =
                0.0f;

            stuck_timers[index] =
                0.0f;

            /*
             * Once the waypoint has been reached,
             * direct movement can be trusted again
             * if the player is visible.
             */
            if (Collision_HasLineOfSight(
                    boss->x,
                    boss->y,
                    player->x,
                    player->y))
            {
                chase_modes[index] =
                    ENEMY_CHASE_DIRECT;
            }
        }

        /*
         * The current waypoint is not producing
         * meaningful progress.
         *
         * Throw it away and try to yield first.
         */
        if (stuck_timers[index] >=
            BOSS_STUCK_TIME)
        {
            boss->path_valid =
                false;

            boss->path_timer =
                0.0f;

            /*
             * Dynamic obstacle fallback.
             */
            if (DistanceBetween(
                    boss->x,
                    boss->y,
                    player->x,
                    player->y) <=
                    BOSS_CONTACT_DISTANCE + 8.0f &&
                Boss_TryYield(
                    index,
                    boss,
                    player))
            {
                boss->boss_state_timer =
                    BOSS_YIELD_TIME;

                stuck_timers[index] =
                    0.0f;

                return;
            }

            stuck_timers[index] =
                0.0f;
        }

        return;
    }


    /*
     * -------------------------------------------------
     * A* FAILURE FALLBACK
     * -------------------------------------------------
     *
     * A* could not produce a waypoint.
     *
     * Try local movement toward the real player,
     * but still monitor meaningful progress.
     */
    float player_distance_before =
        DistanceBetween(
            boss->x,
            boss->y,
            player->x,
            player->y
        );

    bool fallback_moved =
        Boss_MoveDirectToward(
            index,
            boss,
            player->x,
            player->y
        );

    float player_distance_after =
        DistanceBetween(
            boss->x,
            boss->y,
            player->x,
            player->y
        );

    bool meaningful_progress =
        player_distance_before -
        player_distance_after >=
        BOSS_MIN_PROGRESS;

    if (!fallback_moved ||
        !meaningful_progress)
    {
        stuck_timers[index] +=
            GetFrameTime();
    }
    else
    {
        stuck_timers[index] -=
            GetFrameTime() * 2.0f;

        if (stuck_timers[index] < 0.0f)
            stuck_timers[index] = 0.0f;
    }

    if (stuck_timers[index] >=
        BOSS_STUCK_TIME)
    {
        boss->path_valid =
            false;

        boss->path_timer =
            0.0f;

        /*
         * Last-resort dynamic obstacle handling.
         */
        if (DistanceBetween(
                boss->x,
                boss->y,
                player->x,
                player->y) <=
                BOSS_CONTACT_DISTANCE + 8.0f &&
            Boss_TryYield(
                index,
                boss,
                player))
        {
            boss->boss_state_timer =
                BOSS_YIELD_TIME;

            stuck_timers[index] =
                0.0f;

            return;
        }

        stuck_timers[index] =
            0.0f;
    }
}


/*
 * Boss FSM.
 */
static void BossFSM_Update(
    int index,
    Enemy *boss,
    Player *player
)
{
    float dt =
        GetFrameTime();

    float distance =
        DistanceBetween(
            boss->x,
            boss->y,
            player->x,
            player->y
        );

    /*
     * Update timers.
     */
    if (boss->attack_timer > 0.0f)
    {
        boss->attack_timer -= dt;

        if (boss->attack_timer < 0.0f)
            boss->attack_timer = 0.0f;
    }

    /*
     * -------------------------------------------------
     * FIX:
     * Update the boss hurt timer exactly like normal
     * enemies do.
     *
     * Without this, a single hit puts the boss into
     * ENEMY_STATE_HURT permanently because the HURT
     * state returns before the timer can ever reach 0.
     * -------------------------------------------------
     */
    if (boss->hurt_timer > 0.0f)
    {
        boss->hurt_timer -= dt;

        if (boss->hurt_timer < 0.0f)
            boss->hurt_timer = 0.0f;
    }

    if (boss->boss_state_timer > 0.0f)
    {
        boss->boss_state_timer -= dt;

        if (boss->boss_state_timer < 0.0f)
            boss->boss_state_timer = 0.0f;
    }

    if (boss->boss_special_timer > 0.0f)
    {
        boss->boss_special_timer -= dt;

        if (boss->boss_special_timer < 0.0f)
            boss->boss_special_timer = 0.0f;
    }

    if (boss->boss_recovery_timer > 0.0f)
    {
        boss->boss_recovery_timer -= dt;

        if (boss->boss_recovery_timer < 0.0f)
            boss->boss_recovery_timer = 0.0f;
    }

    /*
     * Update health phase.
     */
    Boss_UpdatePhase(
        boss
    );

    /*
     * DEAD
     */
    if (boss->state ==
        ENEMY_STATE_DEAD)
    {
        boss->active = false;

        return;
    }

    /*
     * HURT
     */
    if (boss->state ==
        ENEMY_STATE_HURT)
    {
        if (boss->hurt_timer > 0.0f)
            return;

        boss->state =
            ENEMY_STATE_CHASE;

        boss->boss_state =
            BOSS_STATE_CHASE;

        boss->path_valid =
            false;

        boss->path_timer =
            0.0f;

        stuck_timers[index] =
            0.0f;

        boss_previous_target_distance[index] =
            0.0f;

        return;
    }

    /*
     * If the player is far outside the boss
     * awareness range, return to idle.
     */
    if (distance >
        BOSS_LOSE_RANGE)
    {
        boss->state =
            ENEMY_STATE_IDLE;

        boss->boss_state =
            BOSS_STATE_IDLE;

        boss->path_valid =
            false;

        boss->path_timer =
            0.0f;

        stuck_timers[index] =
            0.0f;

        boss_previous_target_distance[index] =
            0.0f;

        return;
    }

    /*
     * -------------------------------------------------
     * FORCE ENRAGED BEHAVIOR IN PHASE 3
     * -------------------------------------------------
     */
    if (boss->boss_phase ==
            BOSS_PHASE_THREE &&
        boss->boss_state !=
            BOSS_STATE_SPECIAL &&
        boss->boss_state !=
            BOSS_STATE_RECOVER)
    {
        boss->boss_state =
            BOSS_STATE_ENRAGED;
    }

    switch (boss->boss_state)
    {
        /*
         * -------------------------------------------------
         * BOSS IDLE
         * -------------------------------------------------
         */
        case BOSS_STATE_IDLE:
        {
            if (distance <=
                    BOSS_DETECTION_RANGE &&
                Collision_HasLineOfSight(
                    boss->x,
                    boss->y,
                    player->x,
                    player->y))
            {
                boss->state =
                    ENEMY_STATE_CHASE;

                boss->boss_state =
                    BOSS_STATE_CHASE;

                boss->target_x =
                    player->x;

                boss->target_y =
                    player->y;

                boss->path_valid =
                    false;

                boss->path_timer =
                    0.0f;

                stuck_timers[index] =
                    0.0f;

                chase_modes[index] =
                    ENEMY_CHASE_DIRECT;

                boss->boss_state_timer =
                    0.0f;

                boss_previous_target_distance[index] =
                    0.0f;
            }

            break;
        }

        /*
         * -------------------------------------------------
         * BOSS CHASE
         * -------------------------------------------------
         */
        case BOSS_STATE_CHASE:
        {
            /*
             * SPECIAL ATTACK
             */
            if (boss->boss_phase !=
                    BOSS_PHASE_ONE &&
                Boss_CanUseSpecial(boss) &&
                distance <=
                    BOSS_SPECIAL_RANGE &&
                Collision_HasLineOfSight(
                    boss->x,
                    boss->y,
                    player->x,
                    player->y))
            {
                boss->boss_state =
                    BOSS_STATE_SPECIAL;

                boss->state =
                    ENEMY_STATE_ATTACK;

                Boss_PerformSpecial(
                    boss,
                    player
                );

                break;
            }

            /*
             * -------------------------------------------------
             * ENTER COMBAT RANGE
             * -------------------------------------------------
             *
             * Do not yield from CHASE when the boss gets close.
             * That caused a short CHASE -> yield -> CHASE loop
             * around the player.
             *
             * Instead, hand control to ATTACK and let the attack
             * state manage close-range spacing.
             */
            if (distance <=
                    BOSS_ATTACK_RANGE &&
                Collision_HasLineOfSight(
                    boss->x,
                    boss->y,
                    player->x,
                    player->y))
            {
                boss->state =
                    ENEMY_STATE_ATTACK;

                boss->boss_state =
                    BOSS_STATE_ATTACK;

                break;
            }

            Boss_MoveTowardPlayer(
                index,
                boss,
                player
            );

            break;
        }

        /*
         * -------------------------------------------------
         * BOSS ATTACK
         * -------------------------------------------------
         */
        case BOSS_STATE_ATTACK:
        {
            /*
             * Special attack first.
             */
            if (Boss_CanUseSpecial(boss) &&
                boss->boss_phase !=
                    BOSS_PHASE_ONE &&
                distance <=
                    BOSS_SPECIAL_RANGE &&
                Collision_HasLineOfSight(
                    boss->x,
                    boss->y,
                    player->x,
                    player->y))
            {
                boss->boss_state =
                    BOSS_STATE_SPECIAL;

                Boss_PerformSpecial(
                    boss,
                    player
                );

                break;
            }

            /*
             * -------------------------------------------------
             * CLOSE-RANGE SPACING
             * -------------------------------------------------
             *
             * ATTACK is a combat state, not a fixed position.
             * If the boss gets physically too close, move it a
             * small amount away from the player while remaining
             * in ATTACK.
             *
             * Crucially, this does NOT switch back to CHASE.
             * That gives the boss hysteresis and prevents the
             * visible CHASE <-> ATTACK vibration at the boundary.
             */
            if (distance <=
                    BOSS_CONTACT_DISTANCE &&
                Collision_HasLineOfSight(
                    boss->x,
                    boss->y,
                    player->x,
                    player->y))
            {
                if (Boss_TryYield(
                        index,
                        boss,
                        player))
                {
                    boss->state =
                        ENEMY_STATE_ATTACK;

                    boss->boss_state =
                        BOSS_STATE_ATTACK;

                    boss->boss_state_timer =
                        0.0f;

                    return;
                }
            }

            /*
             * Normal attack.
             */
            if (distance <=
                    BOSS_ATTACK_RANGE &&
                Collision_HasLineOfSight(
                    boss->x,
                    boss->y,
                    player->x,
                    player->y))
            {
                if (boss->attack_timer <=
                    0.0f)
                {
                    Player_TakeDamage(
                        BOSS_ATTACK_DAMAGE
                    );

                    Audio_PlayBossAttack();

                    if (boss->boss_phase ==
                        BOSS_PHASE_THREE)
                    {
                        boss->attack_timer =
                            boss->attack_cooldown *
                            0.55f;
                    }
                    else if (boss->boss_phase ==
                             BOSS_PHASE_TWO)
                    {
                        boss->attack_timer =
                            boss->attack_cooldown *
                            0.75f;
                    }
                    else
                    {
                        boss->attack_timer =
                            boss->attack_cooldown;
                    }
                }

                break;
            }

            /*
             * Player moved outside the ATTACK exit range.
             *
             * ATTACK uses a wider exit threshold than its entry
             * threshold. This hysteresis prevents rapid state
             * switching when the distance hovers around 60 px.
             */
            if (distance <=
                BOSS_ATTACK_EXIT_RANGE)
            {
                break;
            }

            /*
             * Player moved far enough away to resume navigation.
             */
            boss->state =
                ENEMY_STATE_CHASE;

            boss->boss_state =
                BOSS_STATE_CHASE;

            boss->path_valid =
                false;

            boss->path_timer =
                0.0f;

            stuck_timers[index] =
                0.0f;

            boss->boss_state_timer =
                0.0f;

            chase_modes[index] =
                ENEMY_CHASE_DIRECT;

            break;
        }

        /*
         * -------------------------------------------------
         * BOSS SPECIAL
         * -------------------------------------------------
         */
        case BOSS_STATE_SPECIAL:
        {
            Boss_PerformSpecial(
                boss,
                player
            );

            break;
        }

        /*
         * -------------------------------------------------
         * BOSS RECOVER
         * -------------------------------------------------
         */
        case BOSS_STATE_RECOVER:
        {
            if (boss->boss_recovery_timer <=
                0.0f)
            {
                if (boss->boss_phase ==
                    BOSS_PHASE_THREE)
                {
                    boss->boss_state =
                        BOSS_STATE_ENRAGED;
                }
                else
                {
                    boss->boss_state =
                        BOSS_STATE_CHASE;
                }

                boss->state =
                    ENEMY_STATE_CHASE;

                /*
                 * CRITICAL:
                 *
                 * The boss must receive a completely
                 * fresh navigation decision after
                 * recovering from a special attack.
                 */
                boss->path_valid =
                    false;

                boss->path_timer =
                    0.0f;

                stuck_timers[index] =
                    0.0f;

                boss->boss_state_timer =
                    0.0f;

                boss_previous_target_distance[index] =
                    0.0f;

                chase_modes[index] =
                    ENEMY_CHASE_DIRECT;
            }

            break;
        }

        /*
         * -------------------------------------------------
         * BOSS ENRAGED
         * -------------------------------------------------
         */
        case BOSS_STATE_ENRAGED:
        {
            /*
             * Phase 3 special.
             */
            if (Boss_CanUseSpecial(boss) &&
                distance <=
                    BOSS_SPECIAL_RANGE &&
                Collision_HasLineOfSight(
                    boss->x,
                    boss->y,
                    player->x,
                    player->y))
            {
                boss->boss_state =
                    BOSS_STATE_SPECIAL;

                boss->state =
                    ENEMY_STATE_ATTACK;

                Boss_PerformSpecial(
                    boss,
                    player
                );

                break;
            }

            boss->speed =
                BOSS_ENRAGED_SPEED;

            if (distance <=
                    BOSS_ATTACK_RANGE &&
                Collision_HasLineOfSight(
                    boss->x,
                    boss->y,
                    player->x,
                    player->y))
            {
                boss->state =
                    ENEMY_STATE_ATTACK;

                boss->boss_state =
                    BOSS_STATE_ATTACK;

                break;
            }

            boss->state =
                ENEMY_STATE_CHASE;

            Boss_MoveTowardPlayer(
                index,
                boss,
                player
            );

            break;
        }
    }
}


void EnemyFSM_Update(
    int index
)
{
    Enemy *enemy =
        Enemy_Get(index);

    Player *player =
        Player_Get();

    if (enemy == 0)
        return;

    if (!enemy->active)
        return;

    /*
     * -------------------------------------------------
     * Boss routing
     * -------------------------------------------------
     */
    if (enemy->is_boss)
    {
        BossFSM_Update(
            index,
            enemy,
            player
        );

        return;
    }

    float distance =
        DistanceBetween(
            enemy->x,
            enemy->y,
            player->x,
            player->y
        );

    float dt =
        GetFrameTime();

    /*
     * Update attack timer.
     */
    if (enemy->attack_timer > 0.0f)
    {
        enemy->attack_timer -= dt;

        if (enemy->attack_timer < 0.0f)
            enemy->attack_timer = 0.0f;
    }

    /*
     * Update hurt timer.
     */
    if (enemy->hurt_timer > 0.0f)
    {
        enemy->hurt_timer -= dt;

        if (enemy->hurt_timer < 0.0f)
            enemy->hurt_timer = 0.0f;
    }

    /*
     * Update path timer.
     */
    if (enemy->path_timer > 0.0f)
    {
        enemy->path_timer -= dt;

        if (enemy->path_timer < 0.0f)
            enemy->path_timer = 0.0f;
    }

    /*
     * Update awareness timer.
     */
    if (enemy->awareness_timer > 0.0f)
    {
        enemy->awareness_timer -= dt;

        if (enemy->awareness_timer < 0.0f)
            enemy->awareness_timer = 0.0f;
    }

    /*
     * DEAD
     */
    if (enemy->state ==
        ENEMY_STATE_DEAD)
    {
        enemy->active = false;

        stuck_timers[index] = 0.0f;

        return;
    }

    /*
     * HURT
     */
    if (enemy->state ==
        ENEMY_STATE_HURT)
    {
        if (enemy->hurt_timer > 0.0f)
            return;

        if (distance <=
            ENEMY_LOSE_RANGE)
        {
            enemy->state =
                ENEMY_STATE_CHASE;

            enemy->path_valid =
                false;

            enemy->path_timer =
                0.0f;

            stuck_timers[index] =
                0.0f;

            chase_modes[index] =
                ENEMY_CHASE_DIRECT;
        }
        else
        {
            enemy->state =
                ENEMY_STATE_IDLE;

            stuck_timers[index] =
                0.0f;
        }

        return;
    }

    /*
     * ATTACK CHECK
     */
    if (distance <=
            ENEMY_ATTACK_RANGE &&
        Collision_HasLineOfSight(
            enemy->x,
            enemy->y,
            player->x,
            player->y))
    {
        enemy->state =
            ENEMY_STATE_ATTACK;
    }

    switch (enemy->state)
    {
        /*
         * -------------------------------------------------
         * IDLE
         * -------------------------------------------------
         */
        case ENEMY_STATE_IDLE:
        {
            if (distance <=
                    ENEMY_DETECTION_RANGE &&
                Collision_HasLineOfSight(
                    enemy->x,
                    enemy->y,
                    player->x,
                    player->y))
            {
                enemy->target_x =
                    player->x;

                enemy->target_y =
                    player->y;

                enemy->awareness_timer =
                    ENEMY_AWARENESS_TIME;

                enemy->path_valid =
                    false;

                enemy->path_timer =
                    0.0f;

                stuck_timers[index] =
                    0.0f;

                chase_modes[index] =
                    ENEMY_CHASE_DIRECT;

                enemy->state =
                    ENEMY_STATE_CHASE;
            }

            break;
        }

        /*
         * -------------------------------------------------
         * CHASE
         * -------------------------------------------------
         */
        case ENEMY_STATE_CHASE:
        {
            if (distance <=
                    ENEMY_ATTACK_RANGE &&
                Collision_HasLineOfSight(
                    enemy->x,
                    enemy->y,
                    player->x,
                    player->y))
            {
                enemy->state =
                    ENEMY_STATE_ATTACK;

                stuck_timers[index] =
                    0.0f;

                break;
            }

            bool has_line_of_sight =
                Collision_HasLineOfSight(
                    enemy->x,
                    enemy->y,
                    player->x,
                    player->y
                );

            /*
             * DIRECT CHASE
             */
            if (chase_modes[index] ==
                ENEMY_CHASE_DIRECT)
            {
                if (has_line_of_sight)
                {
                    enemy->target_x =
                        player->x;

                    enemy->target_y =
                        player->y;

                    enemy->awareness_timer =
                        ENEMY_AWARENESS_TIME;

                    float movement_target_x =
                        player->x;

                    float movement_target_y =
                        player->y;

                    Enemy_GetAdaptiveTarget(
                        enemy,
                        player,
                        &movement_target_x,
                        &movement_target_y
                    );

                    bool moved =
                        Enemy_MoveToward(
                            index,
                            enemy,
                            movement_target_x,
                            movement_target_y
                        );

                    Enemy_UpdateStuckTimer(
                        index,
                        moved
                    );

                    if (Enemy_IsStuck(index))
                    {
                        chase_modes[index] =
                            ENEMY_CHASE_PATH;

                        enemy->path_valid =
                            false;

                        enemy->path_timer =
                            0.0f;

                        stuck_timers[index] =
                            0.0f;
                    }

                    break;
                }

                chase_modes[index] =
                    ENEMY_CHASE_PATH;

                enemy->path_valid =
                    false;

                enemy->path_timer =
                    0.0f;

                stuck_timers[index] =
                    0.0f;
            }

            /*
             * PATH CHASE
             */
            if (chase_modes[index] ==
                ENEMY_CHASE_PATH)
            {
                if (has_line_of_sight)
                {
                    enemy->target_x =
                        player->x;

                    enemy->target_y =
                        player->y;

                    enemy->awareness_timer =
                        ENEMY_AWARENESS_TIME;
                }

                if (enemy->awareness_timer <=
                        0.0f &&
                    distance >=
                        ENEMY_LOSE_RANGE)
                {
                    enemy->path_valid =
                        false;

                    enemy->state =
                        ENEMY_STATE_IDLE;

                    Enemy_ResetNavigation(
                        index,
                        enemy
                    );

                    break;
                }

                if (enemy->path_timer <=
                        0.0f ||
                    !enemy->path_valid)
                {
                    float path_target_x =
                        enemy->target_x;

                    float path_target_y =
                        enemy->target_y;

                    Enemy_GetAdaptiveTarget(
                        enemy,
                        player,
                        &path_target_x,
                        &path_target_y
                    );

                    Enemy_UpdatePath(
                        enemy,
                        path_target_x,
                        path_target_y
                    );

                    enemy->path_timer =
                        ENEMY_PATH_UPDATE_TIME;

                    stuck_timers[index] =
                        0.0f;
                }

                if (enemy->path_valid)
                {
                    bool moved =
                        Enemy_MoveToward(
                            index,
                            enemy,
                            enemy->waypoint_x,
                            enemy->waypoint_y
                        );

                    Enemy_UpdateStuckTimer(
                        index,
                        moved
                    );

                    float waypoint_distance =
                        DistanceBetween(
                            enemy->x,
                            enemy->y,
                            enemy->waypoint_x,
                            enemy->waypoint_y
                        );

                    if (waypoint_distance <
                        7.0f)
                    {
                        enemy->path_timer =
                            0.0f;

                        stuck_timers[index] =
                            0.0f;
                    }

                    if (Enemy_IsStuck(index))
                    {
                        enemy->path_valid =
                            false;

                        enemy->path_timer =
                            0.0f;

                        stuck_timers[index] =
                            0.0f;
                    }
                }
            }

            break;
        }

        /*
         * -------------------------------------------------
         * ATTACK
         * -------------------------------------------------
         */
        case ENEMY_STATE_ATTACK:
        {
            if (distance >
                    ENEMY_ATTACK_RANGE ||
                !Collision_HasLineOfSight(
                    enemy->x,
                    enemy->y,
                    player->x,
                    player->y))
            {
                enemy->state =
                    ENEMY_STATE_CHASE;

                enemy->path_valid =
                    false;

                enemy->path_timer =
                    0.0f;

                stuck_timers[index] =
                    0.0f;

                chase_modes[index] =
                    ENEMY_CHASE_DIRECT;

                break;
            }

            if (enemy->attack_timer <=
                0.0f)
            {
                if (Enemy_ShouldAdaptAttack(
                        enemy))
                {
                    if (enemy->adaptive_decision ==
                        ADAPTIVE_DECISION_ATTACK_AGGRESSIVE)
                    {
                        Player_TakeDamage(
                            ENEMY_ATTACK_DAMAGE
                        );

                        enemy->attack_timer =
                            enemy->attack_cooldown *
                            0.70f;
                    }
                    else
                    {
                        Player_TakeDamage(
                            ENEMY_ATTACK_DAMAGE
                        );

                        enemy->attack_timer =
                            enemy->attack_cooldown *
                            1.25f;
                    }
                }
                else
                {
                    Player_TakeDamage(
                        ENEMY_ATTACK_DAMAGE
                    );

                    enemy->attack_timer =
                        enemy->attack_cooldown;
                }
            }

            break;
        }

        case ENEMY_STATE_HURT:
        {
            break;
        }

        case ENEMY_STATE_DEAD:
        {
            break;
        }
    }
}