#include "adaptive_ai.h"
#include "behavior.h"

#include <stdlib.h>
#include <time.h>

static bool adaptive_ai_initialized = false;


/*
 * ---------------------------------------------------------
 * Utility
 * ---------------------------------------------------------
 */

static float ClampFloat(
    float value,
    float minimum,
    float maximum
)
{
    if (value < minimum)
        return minimum;

    if (value > maximum)
        return maximum;

    return value;
}


/*
 * Generate a random floating-point number
 * between 0.0 and 1.0.
 */
static float RandomFloat(void)
{
    return (float)rand() /
           (float)RAND_MAX;
}


/*
 * ---------------------------------------------------------
 * Initialization
 * ---------------------------------------------------------
 */

void AdaptiveAI_Init(void)
{
    /*
     * Seed the random number generator once.
     *
     * Adaptive AI will use probability rather than
     * deterministic behavior.
     */
    srand(
        (unsigned int)time(NULL)
    );

    adaptive_ai_initialized = true;
}


/*
 * ---------------------------------------------------------
 * Hunter
 * ---------------------------------------------------------
 *
 * Hunter studies the player's movement behavior.
 *
 * Example:
 *
 *     LEFT  = 80%
 *     RIGHT = 15%
 *     UP    =  5%
 *
 * Hunter may predict LEFT, but not with 100%
 * certainty.
 */

static AdaptiveDecisionResult
AdaptiveAI_DecideHunter(
    const PlayerBehavior *profile
)
{
    AdaptiveDecisionResult result = {0};

    result.type =
        ADAPTIVE_AI_HUNTER;

    result.decision =
        ADAPTIVE_DECISION_NONE;

    result.adapted = false;

    if (profile == NULL ||
        !Behavior_HasEnoughData())
    {
        return result;
    }

    /*
     * Find the player's strongest movement
     * tendency.
     */
    float left =
        profile->move_left_probability;

    float right =
        profile->move_right_probability;

    float up =
        profile->move_up_probability;

    float down =
        profile->move_down_probability;

    float highest =
        left;

    AdaptiveDecision predicted =
        ADAPTIVE_DECISION_PREDICT_LEFT;

    if (right > highest)
    {
        highest = right;

        predicted =
            ADAPTIVE_DECISION_PREDICT_RIGHT;
    }

    if (up > highest)
    {
        highest = up;

        predicted =
            ADAPTIVE_DECISION_PREDICT_UP;
    }

    if (down > highest)
    {
        highest = down;

        predicted =
            ADAPTIVE_DECISION_PREDICT_DOWN;
    }

    /*
     * Adaptation strength is limited.
     *
     * The enemy should never perfectly know
     * what the player will do.
     */
    float adaptation =
        ClampFloat(
            highest,
            0.0f,
            0.85f
        );

    /*
     * Add uncertainty.
     *
     * Only use the predicted behavior when
     * the random roll succeeds.
     */
    float random_value =
        RandomFloat();

    result.adaptation_level =
        adaptation;

    result.probability =
        highest;

    if (random_value <= adaptation)
    {
        result.decision =
            predicted;

        result.adapted = true;
    }

    return result;
}


/*
 * ---------------------------------------------------------
 * Guardian
 * ---------------------------------------------------------
 *
 * Guardian currently uses the player's attack
 * frequency as the first adaptation signal.
 *
 * This will later be expanded to:
 *
 *     melee/ranged preference
 *     attack timing
 *     repeated attack patterns
 */

static AdaptiveDecisionResult
AdaptiveAI_DecideGuardian(
    const PlayerBehavior *profile
)
{
    AdaptiveDecisionResult result = {0};

    result.type =
        ADAPTIVE_AI_GUARDIAN;

    result.decision =
        ADAPTIVE_DECISION_NONE;

    result.adapted = false;

    if (profile == NULL ||
        profile->total_attacks < 5)
    {
        return result;
    }

    /*
     * More attacks indicate a more aggressive
     * player.
     */
    if (profile->total_attacks >= 20)
    {
        result.decision =
            ADAPTIVE_DECISION_ATTACK_DEFENSIVE;

        result.adaptation_level =
            0.70f;

        result.probability =
            0.70f;

        if (RandomFloat() <= 0.70f)
        {
            result.adapted = true;
        }
    }
    else
    {
        result.decision =
            ADAPTIVE_DECISION_ATTACK_AGGRESSIVE;

        result.adaptation_level =
            0.40f;

        result.probability =
            0.40f;

        if (RandomFloat() <= 0.40f)
        {
            result.adapted = true;
        }
    }

    return result;
}


/*
 * ---------------------------------------------------------
 * Assassin
 * ---------------------------------------------------------
 *
 * Assassin uses movement tendencies to decide
 * which side to attack from.
 */

static AdaptiveDecisionResult
AdaptiveAI_DecideAssassin(
    const PlayerBehavior *profile
)
{
    AdaptiveDecisionResult result = {0};

    result.type =
        ADAPTIVE_AI_ASSASSIN;

    result.decision =
        ADAPTIVE_DECISION_NONE;

    result.adapted = false;

    if (profile == NULL ||
        !Behavior_HasEnoughData())
    {
        return result;
    }

    float left =
        profile->move_left_probability;

    float right =
        profile->move_right_probability;

    float strongest =
        left;

    if (right > strongest)
        strongest = right;

    /*
     * Assassin chooses the opposite side
     * of the player's strongest movement tendency.
     */
    if (left > right)
    {
        result.decision =
            ADAPTIVE_DECISION_FLANK_RIGHT;
    }
    else
    {
        result.decision =
            ADAPTIVE_DECISION_FLANK_LEFT;
    }

    result.probability =
        strongest;

    result.adaptation_level =
        ClampFloat(
            strongest,
            0.0f,
            0.85f
        );

    if (RandomFloat() <=
        result.adaptation_level)
    {
        result.adapted = true;
    }

    return result;
}


/*
 * ---------------------------------------------------------
 * Boss
 * ---------------------------------------------------------
 *
 * Boss combines multiple adaptive behaviors.
 *
 * For now it uses the strongest movement
 * tendency.
 *
 * Later we will expand this to combine:
 *
 *     movement
 *     attack behavior
 *     positioning
 *     preferred distance
 *     dodge behavior
 */

static AdaptiveDecisionResult
AdaptiveAI_DecideBoss(
    const PlayerBehavior *profile
)
{
    AdaptiveDecisionResult result = {0};

    result.type =
        ADAPTIVE_AI_BOSS;

    result.decision =
        ADAPTIVE_DECISION_NONE;

    result.adapted = false;

    if (profile == NULL ||
        !Behavior_HasEnoughData())
    {
        return result;
    }

    float left =
        profile->move_left_probability;

    float right =
        profile->move_right_probability;

    float up =
        profile->move_up_probability;

    float down =
        profile->move_down_probability;

    float strongest =
        left;

    AdaptiveDecision decision =
        ADAPTIVE_DECISION_FLANK_RIGHT;

    if (right > strongest)
    {
        strongest = right;

        decision =
            ADAPTIVE_DECISION_FLANK_LEFT;
    }

    if (up > strongest)
    {
        strongest = up;

        decision =
            ADAPTIVE_DECISION_CLOSE_DISTANCE;
    }

    if (down > strongest)
    {
        strongest = down;

        decision =
            ADAPTIVE_DECISION_CLOSE_DISTANCE;
    }

    result.decision =
        decision;

    result.probability =
        strongest;

    result.adaptation_level =
        ClampFloat(
            strongest + 0.10f,
            0.0f,
            0.90f
        );

    if (RandomFloat() <=
        result.adaptation_level)
    {
        result.adapted = true;
    }

    return result;
}


/*
 * ---------------------------------------------------------
 * Main Adaptive Decision Function
 * ---------------------------------------------------------
 */

AdaptiveDecisionResult AdaptiveAI_Decide(
    AdaptiveAIType type
)
{
    AdaptiveDecisionResult result = {0};

    /*
     * Make sure initialization happened.
     */
    if (!adaptive_ai_initialized)
    {
        AdaptiveAI_Init();
    }

    const PlayerBehavior *profile =
        Behavior_GetProfile();

    switch (type)
    {
        case ADAPTIVE_AI_HUNTER:
            return AdaptiveAI_DecideHunter(
                profile
            );

        case ADAPTIVE_AI_GUARDIAN:
            return AdaptiveAI_DecideGuardian(
                profile
            );

        case ADAPTIVE_AI_ASSASSIN:
            return AdaptiveAI_DecideAssassin(
                profile
            );

        case ADAPTIVE_AI_BOSS:
            return AdaptiveAI_DecideBoss(
                profile
            );

        default:
            return result;
    }
}


/*
 * ---------------------------------------------------------
 * Debug / UI Names
 * ---------------------------------------------------------
 */

const char *AdaptiveAI_GetTypeName(
    AdaptiveAIType type
)
{
    switch (type)
    {
        case ADAPTIVE_AI_HUNTER:
            return "HUNTER";

        case ADAPTIVE_AI_GUARDIAN:
            return "GUARDIAN";

        case ADAPTIVE_AI_ASSASSIN:
            return "ASSASSIN";

        case ADAPTIVE_AI_BOSS:
            return "BOSS";

        default:
            return "UNKNOWN";
    }
}


const char *AdaptiveAI_GetDecisionName(
    AdaptiveDecision decision
)
{
    switch (decision)
    {
        case ADAPTIVE_DECISION_PREDICT_LEFT:
            return "PREDICT LEFT";

        case ADAPTIVE_DECISION_PREDICT_RIGHT:
            return "PREDICT RIGHT";

        case ADAPTIVE_DECISION_PREDICT_UP:
            return "PREDICT UP";

        case ADAPTIVE_DECISION_PREDICT_DOWN:
            return "PREDICT DOWN";

        case ADAPTIVE_DECISION_ATTACK_AGGRESSIVE:
            return "ATTACK AGGRESSIVE";

        case ADAPTIVE_DECISION_ATTACK_DEFENSIVE:
            return "ATTACK DEFENSIVE";

        case ADAPTIVE_DECISION_FLANK_LEFT:
            return "FLANK LEFT";

        case ADAPTIVE_DECISION_FLANK_RIGHT:
            return "FLANK RIGHT";

        case ADAPTIVE_DECISION_CLOSE_DISTANCE:
            return "CLOSE DISTANCE";

        case ADAPTIVE_DECISION_KEEP_DISTANCE:
            return "KEEP DISTANCE";

        case ADAPTIVE_DECISION_NONE:
        default:
            return "NONE";
    }
}