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
 * How far from the nearest enemy the player usually fights.
 */
typedef enum
{
    FIGHT_STYLE_UNKNOWN,

    FIGHT_STYLE_CLOSE,
    FIGHT_STYLE_MID,
    FIGHT_STYLE_FAR

} FightStyle;


typedef struct
{
    /*
     * -----------------------------------------------------
     * Attack Behavior
     * -----------------------------------------------------
     */

    int total_attacks;

    /*
     * Attack intensity over the recent past. Each attack
     * adds 1.0 and the value fades over time.
     */
    float recent_attack_score;

    /*
     * Attacks made while an enemy was close enough to
     * measure, split by distance from that enemy:
     *
     *   close - almost touching
     *   mid   - normal fighting distance
     *   far   - at the edge of the player's reach
     *
     * close + mid + far == attacks_measured
     */
    int attacks_measured;

    int close_attacks;
    int mid_attacks;
    int far_attacks;


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

    /* Share of measured attacks that were close range. */
    float close_attack_ratio;


    /*
     * -----------------------------------------------------
     * Profile State
     * -----------------------------------------------------
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
 *
 * For EVENT_PLAYER_ATTACK, the event's value is the distance
 * in pixels to the nearest enemy, or a negative number when
 * no enemy was near.
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

float Behavior_GetMoveLeftProbability(void);
float Behavior_GetMoveRightProbability(void);
float Behavior_GetMoveUpProbability(void);
float Behavior_GetMoveDownProbability(void);

float Behavior_GetDodgeLeftProbability(void);
float Behavior_GetDodgeRightProbability(void);
float Behavior_GetDodgeUpProbability(void);
float Behavior_GetDodgeDownProbability(void);


/*
 * ---------------------------------------------------------
 * FIGHT STYLE
 * ---------------------------------------------------------
 */

/*
 * The distance band the player attacks from most often.
 * FIGHT_STYLE_UNKNOWN until enough attacks were measured.
 */
FightStyle Behavior_GetFightStyle(void);

/*
 * Share (0.0 to 1.0) of measured attacks that fall in the
 * dominant band. 0.0 while the style is unknown.
 */
float Behavior_GetFightStyleShare(void);


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
 * Advance time-based statistics (attack score decay).
 * Call once per gameplay frame.
 */
void Behavior_Tick(
    float dt
);


/*
 * Check a profile without applying it.
 */
bool Behavior_IsValid(
    const PlayerBehavior *profile
);


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
    const PlayerBehavior *saved
);

#endif