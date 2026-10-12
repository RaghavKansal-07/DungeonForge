#include "behavior.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

/*
 * Time constant for forgetting attacks. After this many
 * seconds an attack's weight has fallen to about 37%.
 */
#define BEHAVIOR_ATTACK_MEMORY_SECONDS 15.0f

/*
 * Attack distance bands, in pixels between the player's and
 * the enemy's centers. Bodies touch at about 40 px.
 */
#define BEHAVIOR_CLOSE_ATTACK_DISTANCE 52.0f
#define BEHAVIOR_MID_ATTACK_DISTANCE 66.0f

/* Measured attacks needed before a fight style is reported. */
#define BEHAVIOR_MIN_STYLE_SAMPLES 6

/*
 * ---------------------------------------------------------
 * PLAYER BEHAVIOR PROFILE
 * ---------------------------------------------------------
 */

static PlayerBehavior behavior;

/*
 * ---------------------------------------------------------
 * INTERNAL HELPERS
 * ---------------------------------------------------------
 */

/*
 * Recalculate movement probabilities from the
 * accumulated movement counters.
 */
static void Behavior_UpdateMovementProbabilities(void)
{
    if (behavior.total_movements <= 0)
    {
        behavior.move_left_probability = 0.0f;
        behavior.move_right_probability = 0.0f;
        behavior.move_up_probability = 0.0f;
        behavior.move_down_probability = 0.0f;

        return;
    }

    float total =
        (float)behavior.total_movements;

    behavior.move_left_probability =
        (float)behavior.move_left_count / total;

    behavior.move_right_probability =
        (float)behavior.move_right_count / total;

    behavior.move_up_probability =
        (float)behavior.move_up_count / total;

    behavior.move_down_probability =
        (float)behavior.move_down_count / total;
}

/*
 * Recalculate dodge probabilities from the
 * accumulated dodge counters.
 */
static void Behavior_UpdateDodgeProbabilities(void)
{
    if (behavior.total_dodges <= 0)
    {
        behavior.dodge_left_probability = 0.0f;
        behavior.dodge_right_probability = 0.0f;
        behavior.dodge_up_probability = 0.0f;
        behavior.dodge_down_probability = 0.0f;

        return;
    }

    float total =
        (float)behavior.total_dodges;

    behavior.dodge_left_probability =
        (float)behavior.dodge_left_count / total;

    behavior.dodge_right_probability =
        (float)behavior.dodge_right_count / total;

    behavior.dodge_up_probability =
        (float)behavior.dodge_up_count / total;

    behavior.dodge_down_probability =
        (float)behavior.dodge_down_count / total;
}

/*
 * Recalculate the attack-distance statistics.
 */
static void Behavior_UpdateAttackStats(void)
{
    if (behavior.attacks_measured > 0)
    {
        behavior.close_attack_ratio =
            (float)behavior.close_attacks /
            (float)behavior.attacks_measured;
    }
    else
    {
        behavior.close_attack_ratio = 0.0f;
    }
}

/*
 * Recalculate average damage taken per damage event.
 */
static void Behavior_UpdateAverageDamage(void)
{
    if (behavior.damage_events > 0)
    {
        behavior.average_damage_taken =
            (float)behavior.total_damage_taken /
            (float)behavior.damage_events;
    }
    else
    {
        behavior.average_damage_taken = 0.0f;
    }
}

/*
 * ---------------------------------------------------------
 * INITIALIZATION
 * ---------------------------------------------------------
 */

void Behavior_Init(void)
{
    /*
     * Clear the complete behavior profile.
     */
    memset(
        &behavior,
        0,
        sizeof(PlayerBehavior));

    behavior.initialized = true;
}

/*
 * ---------------------------------------------------------
 * EVENT PROCESSING
 * ---------------------------------------------------------
 */

void Behavior_Update(void)
{
    GameEvent event;

    /*
     * Consume every event currently waiting
     * in the event queue.
     */
    while (Events_Pop(&event))
    {
        switch (event.type)
        {
            /*
             * -------------------------------------------------
             * PLAYER ATTACK
             * -------------------------------------------------
             *
             * The event value is the distance to the nearest
             * enemy. A negative (or invalid) value means there
             * was no enemy to measure against.
             */

        case EVENT_PLAYER_ATTACK:
        {
            behavior.total_attacks++;
            behavior.recent_attack_score += 1.0f;

            if (event.value >= 0.0f)
            {
                behavior.attacks_measured++;

                if (event.value <=
                    BEHAVIOR_CLOSE_ATTACK_DISTANCE)
                {
                    behavior.close_attacks++;
                }
                else if (event.value <=
                         BEHAVIOR_MID_ATTACK_DISTANCE)
                {
                    behavior.mid_attacks++;
                }
                else
                {
                    behavior.far_attacks++;
                }
            }

            break;
        }

            /*
             * -------------------------------------------------
             * PLAYER MOVEMENT
             * -------------------------------------------------
             */

        case EVENT_PLAYER_MOVE:
        {
            behavior.total_movements++;

            switch (event.move_direction)
            {
            case MOVE_LEFT:
                behavior.move_left_count++;
                break;

            case MOVE_RIGHT:
                behavior.move_right_count++;
                break;

            case MOVE_UP:
                behavior.move_up_count++;
                break;

            case MOVE_DOWN:
                behavior.move_down_count++;
                break;

            case MOVE_NONE:
            default:
                break;
            }

            break;
        }

            /*
             * -------------------------------------------------
             * PLAYER DODGE
             * -------------------------------------------------
             */

        case EVENT_PLAYER_DODGE:
        {
            behavior.total_dodges++;

            switch (event.dodge_direction)
            {
            case DODGE_LEFT:
                behavior.dodge_left_count++;
                break;

            case DODGE_RIGHT:
                behavior.dodge_right_count++;
                break;

            case DODGE_UP:
                behavior.dodge_up_count++;
                break;

            case DODGE_DOWN:
                behavior.dodge_down_count++;
                break;

            case DODGE_NONE:
            default:
                break;
            }

            break;
        }

            /*
             * -------------------------------------------------
             * PLAYER DAMAGE
             * -------------------------------------------------
             */

        case EVENT_PLAYER_DAMAGE:
        {
            behavior.damage_events++;

            if (event.value > 0.0f)
            {
                behavior.total_damage_taken +=
                    (int)event.value;
            }

            break;
        }

            /*
             * -------------------------------------------------
             * PLAYER HEAL
             * -------------------------------------------------
             */

        case EVENT_PLAYER_HEAL:
        {
            if (event.value > 0.0f)
            {
                behavior.total_healing +=
                    (int)event.value;
            }

            break;
        }

            /*
             * -------------------------------------------------
             * OTHER EVENTS
             * -------------------------------------------------
             */

        case EVENT_ENEMY_DAMAGED:
        case EVENT_NONE:
        default:
            break;
        }
    }

    /*
     * ---------------------------------------------------------
     * UPDATE DERIVED STATISTICS
     * ---------------------------------------------------------
     */

    Behavior_UpdateMovementProbabilities();

    Behavior_UpdateDodgeProbabilities();

    Behavior_UpdateAttackStats();

    Behavior_UpdateAverageDamage();
}

/*
 * ---------------------------------------------------------
 * PROFILE ACCESS
 * ---------------------------------------------------------
 */

const PlayerBehavior *Behavior_GetProfile(void)
{
    return &behavior;
}

void Behavior_Tick(
    float dt)
{
    if (dt <= 0.0f)
        return;

    if (dt > 0.1f)
        dt = 0.1f;

    behavior.recent_attack_score *=
        expf(-dt / BEHAVIOR_ATTACK_MEMORY_SECONDS);

    if (behavior.recent_attack_score < 0.001f)
        behavior.recent_attack_score = 0.0f;
}

/*
 * ---------------------------------------------------------
 * MOVEMENT PROBABILITY HELPERS
 * ---------------------------------------------------------
 */

float Behavior_GetMoveLeftProbability(void)
{
    return behavior.move_left_probability;
}

float Behavior_GetMoveRightProbability(void)
{
    return behavior.move_right_probability;
}

float Behavior_GetMoveUpProbability(void)
{
    return behavior.move_up_probability;
}

float Behavior_GetMoveDownProbability(void)
{
    return behavior.move_down_probability;
}

/*
 * ---------------------------------------------------------
 * DODGE PROBABILITY HELPERS
 * ---------------------------------------------------------
 */

float Behavior_GetDodgeLeftProbability(void)
{
    return behavior.dodge_left_probability;
}

float Behavior_GetDodgeRightProbability(void)
{
    return behavior.dodge_right_probability;
}

float Behavior_GetDodgeUpProbability(void)
{
    return behavior.dodge_up_probability;
}

float Behavior_GetDodgeDownProbability(void)
{
    return behavior.dodge_down_probability;
}

/*
 * ---------------------------------------------------------
 * FIGHT STYLE
 * ---------------------------------------------------------
 */

FightStyle Behavior_GetFightStyle(void)
{
    if (behavior.attacks_measured <
        BEHAVIOR_MIN_STYLE_SAMPLES)
    {
        return FIGHT_STYLE_UNKNOWN;
    }

    /*
     * Ties prefer the closer band.
     */
    if (behavior.close_attacks >= behavior.mid_attacks &&
        behavior.close_attacks >= behavior.far_attacks)
    {
        return FIGHT_STYLE_CLOSE;
    }

    if (behavior.mid_attacks >= behavior.far_attacks)
        return FIGHT_STYLE_MID;

    return FIGHT_STYLE_FAR;
}

float Behavior_GetFightStyleShare(void)
{
    FightStyle style =
        Behavior_GetFightStyle();

    if (style == FIGHT_STYLE_UNKNOWN ||
        behavior.attacks_measured <= 0)
    {
        return 0.0f;
    }

    int dominant = 0;

    switch (style)
    {
    case FIGHT_STYLE_CLOSE:
        dominant = behavior.close_attacks;
        break;

    case FIGHT_STYLE_MID:
        dominant = behavior.mid_attacks;
        break;

    case FIGHT_STYLE_FAR:
        dominant = behavior.far_attacks;
        break;

    case FIGHT_STYLE_UNKNOWN:
    default:
        break;
    }

    return (float)dominant /
           (float)behavior.attacks_measured;
}

/*
 * ---------------------------------------------------------
 * DATA SUFFICIENCY
 * ---------------------------------------------------------
 *
 * We don't want enemies adapting aggressively from only
 * one or two player actions.
 */

bool Behavior_HasEnoughData(void)
{
    const int minimum_observations = 10;

    int total_observations =
        behavior.total_movements +
        behavior.total_dodges +
        behavior.total_attacks;

    return total_observations >=
           minimum_observations;
}

/*
 * ---------------------------------------------------------
 * SAVE / LOAD SUPPORT
 * ---------------------------------------------------------
 */

bool Behavior_IsValid(
    const PlayerBehavior *saved)
{
    if (saved == NULL)
        return false;

    if (!isfinite(saved->recent_attack_score) ||
        saved->recent_attack_score < 0.0f)
    {
        return false;
    }

    const int counters[] =
        {
            saved->total_attacks,
            saved->attacks_measured,
            saved->close_attacks,
            saved->mid_attacks,
            saved->far_attacks,
            saved->total_movements,
            saved->move_left_count,
            saved->move_right_count,
            saved->move_up_count,
            saved->move_down_count,
            saved->total_dodges,
            saved->dodge_left_count,
            saved->dodge_right_count,
            saved->dodge_up_count,
            saved->dodge_down_count,
            saved->total_damage_taken,
            saved->damage_events,
            saved->total_healing};

    const int counter_count =
        sizeof(counters) / sizeof(counters[0]);

    for (int i = 0; i < counter_count; i++)
    {
        if (counters[i] < 0 ||
            counters[i] > 100000000)
        {
            return false;
        }
    }

    /*
     * A direction count can never exceed its total.
     */
    if (saved->move_left_count > saved->total_movements ||
        saved->move_right_count > saved->total_movements ||
        saved->move_up_count > saved->total_movements ||
        saved->move_down_count > saved->total_movements ||
        saved->dodge_left_count > saved->total_dodges ||
        saved->dodge_right_count > saved->total_dodges ||
        saved->dodge_up_count > saved->total_dodges ||
        saved->dodge_down_count > saved->total_dodges)
    {
        return false;
    }

    /*
     * Measured attacks are a subset of all attacks, and the
     * three distance bands must add up to exactly that subset.
     */
    if (saved->attacks_measured > saved->total_attacks)
        return false;

    if (saved->close_attacks +
            saved->mid_attacks +
            saved->far_attacks !=
        saved->attacks_measured)
    {
        return false;
    }

    return true;
}

bool Behavior_Restore(
    const PlayerBehavior *saved)
{
    if (!Behavior_IsValid(saved))
        return false;

    behavior = *saved;
    behavior.initialized = true;

    if (behavior.recent_attack_score > 100.0f)
        behavior.recent_attack_score = 100.0f;

    /*
     * Never trust saved derived values.
     */
    Behavior_UpdateMovementProbabilities();
    Behavior_UpdateDodgeProbabilities();
    Behavior_UpdateAttackStats();
    Behavior_UpdateAverageDamage();

    return true;
}