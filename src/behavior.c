#include "behavior.h"

#include <string.h>

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
             */

        case EVENT_PLAYER_ATTACK:
        {
            behavior.total_attacks++;

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
                /*
                 * No valid movement direction.
                 */
                break;
            }

            break;
        }

            /*
             * -------------------------------------------------
             * PLAYER DODGE
             * -------------------------------------------------
             *
             * The player generates this event when SHIFT
             * successfully starts a dodge.
             *
             * The dodge direction is stored as a primary
             * cardinal direction.
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
                /*
                 * No valid dodge direction.
                 */
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
            /*
             * These events are not currently used
             * by the behavior analyzer.
             */
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

    /*
     * Calculate average damage taken per damage event.
     */
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
 * PROFILE ACCESS
 * ---------------------------------------------------------
 */

const PlayerBehavior *Behavior_GetProfile(void)
{
    return &behavior;
}

bool Behavior_Restore(
    const PlayerBehavior *saved)
{
    if (saved == NULL)
        return false;

    /*
     * Reject impossible counters.
     */
    const int counters[] =
        {
            saved->total_attacks,
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
        if (counters[i] < 0)
            return false;
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

    behavior = *saved;
    behavior.initialized = true;

    /*
     * Never trust saved derived values.
     */
    Behavior_UpdateMovementProbabilities();
    Behavior_UpdateDodgeProbabilities();

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

    return true;
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
 * DATA SUFFICIENCY
 * ---------------------------------------------------------
 *
 * We don't want enemies adapting aggressively from only
 * one or two player actions.
 *
 * A minimum number of movement observations is therefore
 * required before the adaptive AI considers the movement
 * profile reliable enough to use.
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