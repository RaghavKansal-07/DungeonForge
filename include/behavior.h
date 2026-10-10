#ifndef BEHAVIOR_H
#define BEHAVIOR_H

#include <stdbool.h>

#include "events.h"

/*
 * ---------------------------------------------------------
 * PLAYER BEHAVIOR PROFILE
 * ---------------------------------------------------------
 *
 * Stores statistical information about how the player
 * behaves during the current dungeon run.
 *
 * The profile is built from gameplay events.
 *
 * No machine learning is used.
 * The system uses counters, averages and probabilities.
 */

/*
 * ---------------------------------------------------------
 * BEHAVIOR PROFILE
 * ---------------------------------------------------------
 */

typedef struct
{
    /*
     * -----------------------------------------------------
     * Attack Behavior
     * -----------------------------------------------------
     */

    int total_attacks;

    /*
     * -----------------------------------------------------
     * Movement Behavior
     * -----------------------------------------------------
     */

    int total_movements;

    int move_left_count;
    int move_right_count;
    int move_up_count;
    int move_down_count;

    /*
     * -----------------------------------------------------
     * Dodge Behavior
     * -----------------------------------------------------
     */

    int total_dodges;

    int dodge_left_count;
    int dodge_right_count;
    int dodge_up_count;
    int dodge_down_count;

    /*
     * -----------------------------------------------------
     * Damage / Aggression Behavior
     * -----------------------------------------------------
     */

    int total_damage_taken;

    int damage_events;

    int total_healing;

    /*
     * -----------------------------------------------------
     * Derived Probabilities
     * -----------------------------------------------------
     *
     * Values are in the range [0.0, 1.0].
     */

    float move_left_probability;
    float move_right_probability;
    float move_up_probability;
    float move_down_probability;

    float dodge_left_probability;
    float dodge_right_probability;
    float dodge_up_probability;
    float dodge_down_probability;

    /*
     * -----------------------------------------------------
     * Combat Statistics
     * -----------------------------------------------------
     */

    float average_damage_taken;

    /*
     * -----------------------------------------------------
     * Profile State
     * -----------------------------------------------------
     *
     * Indicates whether enough information has been
     * collected to make meaningful adaptive decisions.
     */

    bool initialized;

} PlayerBehavior;

/*
 * ---------------------------------------------------------
 * BEHAVIOR SYSTEM
 * ---------------------------------------------------------
 */

/*
 * Reset the player's behavior profile.
 */
void Behavior_Init(void);

/*
 * Process all pending gameplay events.
 *
 * Events are consumed from the global event queue and
 * converted into behavioral statistics.
 */
void Behavior_Update(void);

/*
 * Get the current player behavior profile.
 */
const PlayerBehavior *Behavior_GetProfile(void);

/*
 * ---------------------------------------------------------
 * PROBABILITY HELPERS
 * ---------------------------------------------------------
 */

/*
 * Get the probability that the player moves left.
 */
float Behavior_GetMoveLeftProbability(void);

/*
 * Get the probability that the player moves right.
 */
float Behavior_GetMoveRightProbability(void);

/*
 * Get the probability that the player moves up.
 */
float Behavior_GetMoveUpProbability(void);

/*
 * Get the probability that the player moves down.
 */
float Behavior_GetMoveDownProbability(void);

/*
 * Get the probability that the player dodges left.
 */
float Behavior_GetDodgeLeftProbability(void);

/*
 * Get the probability that the player dodges right.
 */
float Behavior_GetDodgeRightProbability(void);

/*
 * Get the probability that the player dodges up.
 */
float Behavior_GetDodgeUpProbability(void);

/*
 * Get the probability that the player dodges down.
 */
float Behavior_GetDodgeDownProbability(void);

/*
 * ---------------------------------------------------------
 * DEBUG
 * ---------------------------------------------------------
 */

/*
 * Returns true when the profile contains enough
 * information for adaptive AI decisions.
 */
bool Behavior_HasEnoughData(void);

/*
 * Restore a previously saved behavior profile.
 *
 * Counters are validated and every derived value
 * (probabilities, average damage) is recalculated,
 * so a corrupt save cannot inject bad statistics.
 *
 * Returns false (and leaves the current profile
 * unchanged) if the saved data is invalid.
 */
bool Behavior_Restore(
    const PlayerBehavior *saved);

#endif