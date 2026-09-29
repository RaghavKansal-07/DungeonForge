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
        sizeof(PlayerBehavior)
    );

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
             * Other events
             * -------------------------------------------------
             */

            case EVENT_PLAYER_DODGE:
            case EVENT_ENEMY_DAMAGED:
            case EVENT_NONE:
            default:
                /*
                 * These events will be handled by later
                 * behavior-analysis features.
                 */
                break;
        }
    }

    /*
     * Recalculate all derived movement probabilities
     * after processing the current batch of events.
     */
    Behavior_UpdateMovementProbabilities();

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


/*
 * ---------------------------------------------------------
 * PROBABILITY HELPERS
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
    const int minimum_movements = 10;

    return behavior.total_movements >=
           minimum_movements;
}