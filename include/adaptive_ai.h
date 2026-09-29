#ifndef ADAPTIVE_AI_H
#define ADAPTIVE_AI_H

#include <stdbool.h>

/*
 * Enemy archetypes.
 *
 * Each archetype uses the player's behavioral profile
 * differently.
 */
typedef enum
{
    ADAPTIVE_AI_HUNTER,
    ADAPTIVE_AI_GUARDIAN,
    ADAPTIVE_AI_ASSASSIN,
    ADAPTIVE_AI_BOSS

} AdaptiveAIType;


/*
 * Result of an adaptive AI decision.
 *
 * The enemy system will later use this information
 * to modify its behavior.
 */
typedef enum
{
    ADAPTIVE_DECISION_NONE,

    /*
     * Predict where the player will move.
     */
    ADAPTIVE_DECISION_PREDICT_LEFT,
    ADAPTIVE_DECISION_PREDICT_RIGHT,
    ADAPTIVE_DECISION_PREDICT_UP,
    ADAPTIVE_DECISION_PREDICT_DOWN,

    /*
     * Change attack behavior.
     */
    ADAPTIVE_DECISION_ATTACK_AGGRESSIVE,
    ADAPTIVE_DECISION_ATTACK_DEFENSIVE,

    /*
     * Change positioning.
     */
    ADAPTIVE_DECISION_FLANK_LEFT,
    ADAPTIVE_DECISION_FLANK_RIGHT,
    ADAPTIVE_DECISION_CLOSE_DISTANCE,
    ADAPTIVE_DECISION_KEEP_DISTANCE

} AdaptiveDecision;


/*
 * Result returned by the adaptive AI system.
 */
typedef struct
{
    AdaptiveAIType type;

    AdaptiveDecision decision;

    /*
     * Probability that the adaptive behavior
     * was selected.
     */
    float probability;

    /*
     * How strongly this enemy is adapting
     * to the player's behavior.
     *
     * Range:
     *     0.0 = no adaptation
     *     1.0 = maximum adaptation
     */
    float adaptation_level;

    /*
     * Whether enough player behavior data
     * exists to make an adaptive decision.
     */
    bool adapted;

} AdaptiveDecisionResult;


/*
 * Initialize the adaptive AI system.
 *
 * Call once when starting a new run.
 */
void AdaptiveAI_Init(void);


/*
 * Generate an adaptive decision for an enemy.
 *
 * The enemy type determines which aspects of
 * the player's behavior are considered.
 */
AdaptiveDecisionResult AdaptiveAI_Decide(
    AdaptiveAIType type
);


/*
 * Get a human-readable name for an enemy type.
 */
const char *AdaptiveAI_GetTypeName(
    AdaptiveAIType type
);


/*
 * Get a human-readable name for an
 * adaptive decision.
 */
const char *AdaptiveAI_GetDecisionName(
    AdaptiveDecision decision
);

#endif