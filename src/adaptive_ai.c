#include "adaptive_ai.h"
#include "behavior.h"

#include "rng.h"
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
    float maximum)
{
    if (value < minimum)
        return minimum;

    if (value > maximum)
        return maximum;

    return value;
}

/*
 * The adaptive AI owns its generator, separate from
 * dungeon generation.
 */
static Rng ai_rng = {1u};

static float RandomFloat(void)
{
    return Rng_Float(&ai_rng);
}

/*
 * ---------------------------------------------------------
 * Directional Probability Selection
 * ---------------------------------------------------------
 *
 * Select one direction according to the player's
 * observed probability distribution.
 *
 * This is intentionally probabilistic.
 *
 * Example:
 *
 *     LEFT   0.70
 *     RIGHT  0.20
 *     UP     0.07
 *     DOWN   0.03
 *
 * The AI therefore does not perfectly predict the player.
 *
 * Important:
 * A direction with zero probability must NEVER be selected.
 */
static AdaptiveDecision SelectPredictedDirection(
    float left,
    float right,
    float up,
    float down)
{
    float total =
        left +
        right +
        up +
        down;

    if (total <= 0.0f)
    {
        return ADAPTIVE_DECISION_NONE;
    }

    float random_value =
        RandomFloat() * total;

    /*
     * LEFT
     */
    if (left > 0.0f &&
        random_value < left)
    {
        return ADAPTIVE_DECISION_PREDICT_LEFT;
    }

    random_value -= left;

    /*
     * RIGHT
     */
    if (right > 0.0f &&
        random_value < right)
    {
        return ADAPTIVE_DECISION_PREDICT_RIGHT;
    }

    random_value -= right;

    /*
     * UP
     */
    if (up > 0.0f &&
        random_value < up)
    {
        return ADAPTIVE_DECISION_PREDICT_UP;
    }

    random_value -= up;

    /*
     * DOWN
     */
    if (down > 0.0f &&
        random_value < down)
    {
        return ADAPTIVE_DECISION_PREDICT_DOWN;
    }

    /*
     * Floating-point values can leave a tiny residual
     * after subtraction. Never select an unseen direction
     * as a fallback.
     *
     * Return the last available observed direction.
     */
    if (down > 0.0f)
    {
        return ADAPTIVE_DECISION_PREDICT_DOWN;
    }

    if (up > 0.0f)
    {
        return ADAPTIVE_DECISION_PREDICT_UP;
    }

    if (right > 0.0f)
    {
        return ADAPTIVE_DECISION_PREDICT_RIGHT;
    }

    if (left > 0.0f)
    {
        return ADAPTIVE_DECISION_PREDICT_LEFT;
    }

    return ADAPTIVE_DECISION_NONE;
}

/*
 * ---------------------------------------------------------
 * Initialization
 * ---------------------------------------------------------
 */

void AdaptiveAI_Init(void)
{
    /*
     * Seed once from the clock. Adaptive AI is meant
     * to be unpredictable between runs.
     */
    Rng_Seed(
        &ai_rng,
        (uint32_t)time(NULL));

    adaptive_ai_initialized = true;
}

void AdaptiveAI_Seed(unsigned int seed)
{
    Rng_Seed(&ai_rng, seed);

    adaptive_ai_initialized = true;
}
/*
 * ---------------------------------------------------------
 * Hunter
 * ---------------------------------------------------------
 *
 * Hunter studies the player's dodge behavior first.
 *
 * If enough dodge observations exist, the Hunter predicts
 * the player's future dodge direction probabilistically.
 *
 * If the player has not dodged enough times yet, the
 * Hunter falls back to the existing movement profile.
 */
static AdaptiveDecisionResult
AdaptiveAI_DecideHunter(
    const PlayerBehavior *profile)
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
     * -----------------------------------------------------
     * Prefer Dodge Behavior
     * -----------------------------------------------------
     */

    if (profile->total_dodges >= 3)
    {
        float left =
            profile->dodge_left_probability;

        float right =
            profile->dodge_right_probability;

        float up =
            profile->dodge_up_probability;

        float down =
            profile->dodge_down_probability;

        /*
         * Find the strongest observed tendency.
         */
        float highest =
            left;

        if (right > highest)
            highest = right;

        if (up > highest)
            highest = up;

        if (down > highest)
            highest = down;

        /*
         * The Hunter should not adapt with absolute
         * certainty.
         *
         * Even an 81% player tendency becomes at most
         * an 85% adaptation chance.
         */
        float adaptation =
            ClampFloat(
                highest,
                0.0f,
                0.85f);

        result.adaptation_level =
            adaptation;

        result.probability =
            highest;

        /*
         * Decide whether the Hunter adapts this cycle.
         */
        if (RandomFloat() <= adaptation)
        {
            /*
             * Choose according to the complete
             * probability distribution.
             */
            result.decision =
                SelectPredictedDirection(
                    left,
                    right,
                    up,
                    down);

            result.adapted = true;
        }

        return result;
    }

    /*
     * -----------------------------------------------------
     * Movement Fallback
     * -----------------------------------------------------
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

    if (right > highest)
        highest = right;

    if (up > highest)
        highest = up;

    if (down > highest)
        highest = down;

    float adaptation =
        ClampFloat(
            highest,
            0.0f,
            0.85f);

    result.adaptation_level =
        adaptation;

    result.probability =
        highest;

    if (RandomFloat() <= adaptation)
    {
        result.decision =
            SelectPredictedDirection(
                left,
                right,
                up,
                down);

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
 * More attacks indicate a more aggressive player.
 */
static AdaptiveDecisionResult
AdaptiveAI_DecideGuardian(
    const PlayerBehavior *profile)
{
    AdaptiveDecisionResult result = {0};

    result.type =
        ADAPTIVE_AI_GUARDIAN;

    result.decision =
        ADAPTIVE_DECISION_NONE;

    result.adapted = false;

    /*
     * Not enough recent attacks to judge the player.
     */
    if (profile == NULL ||
        profile->recent_attack_score < 5.0f)
    {
        return result;
    }

    float level;

    if (profile->recent_attack_score >= 20.0f)
    {
        /*
         * Heavy recent attacking: defend. The more
         * intense the attacking, the stronger the reaction.
         */
        result.decision =
            ADAPTIVE_DECISION_ATTACK_DEFENSIVE;

        level =
            ClampFloat(
                0.70f +
                    (profile->recent_attack_score - 20.0f) *
                        0.01f,
                0.70f,
                0.85f);
    }
    else
    {
        result.decision =
            ADAPTIVE_DECISION_ATTACK_AGGRESSIVE;

        level = 0.40f;
    }

    result.adaptation_level = level;
    result.probability = level;

    if (RandomFloat() <= level)
        result.adapted = true;

    return result;
}

/*
 * ---------------------------------------------------------
 * Assassin
 * ---------------------------------------------------------
 *
 * Assassin uses dodge behavior when enough dodge
 * observations exist.
 *
 * The Assassin attempts to attack from the side
 * opposite to the player's strongest dodge tendency.
 */
static AdaptiveDecisionResult
AdaptiveAI_DecideAssassin(
    const PlayerBehavior *profile)
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

    /*
     * Prefer dodge behavior. Fall back to movement
     * when there are too few dodges.
     */
    float left;
    float right;
    float up;
    float down;

    if (profile->total_dodges >= 3)
    {
        left = profile->dodge_left_probability;
        right = profile->dodge_right_probability;
        up = profile->dodge_up_probability;
        down = profile->dodge_down_probability;
    }
    else
    {
        left = profile->move_left_probability;
        right = profile->move_right_probability;
        up = profile->move_up_probability;
        down = profile->move_down_probability;
    }

    float horizontal =
        (left > right) ? left : right;

    float vertical =
        (up > down) ? up : down;

    float strongest =
        (horizontal > vertical)
            ? horizontal
            : vertical;

    /*
     * No observed tendency: nothing to adapt to.
     */
    if (strongest <= 0.0f)
        return result;

    AdaptiveDecision decision;

    if (vertical > horizontal)
    {
        /*
         * A vertical habit cannot be flanked sideways.
         * Rush the player instead.
         */
        decision =
            ADAPTIVE_DECISION_CLOSE_DISTANCE;
    }
    else if (left >= right)
    {
        decision =
            ADAPTIVE_DECISION_FLANK_RIGHT;
    }
    else
    {
        decision =
            ADAPTIVE_DECISION_FLANK_LEFT;
    }

    result.probability =
        strongest;

    result.adaptation_level =
        ClampFloat(
            strongest,
            0.0f,
            0.85f);

    if (RandomFloat() <=
        result.adaptation_level)
    {
        result.decision =
            decision;

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
 * Dodge behavior has priority because it gives the Boss
 * information about how the player reacts during combat.
 *
 * Movement behavior remains the fallback.
 */
static AdaptiveDecisionResult
AdaptiveAI_DecideBoss(
    const PlayerBehavior *profile)
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

    /*
     * -----------------------------------------------------
     * Dodge-Based Adaptation
     * -----------------------------------------------------
     */

    if (profile->total_dodges >= 3)
    {
        float left =
            profile->dodge_left_probability;

        float right =
            profile->dodge_right_probability;

        float up =
            profile->dodge_up_probability;

        float down =
            profile->dodge_down_probability;

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
                0.90f);

        if (RandomFloat() <=
            result.adaptation_level)
        {
            result.decision =
                decision;

            result.adapted = true;
        }

        return result;
    }

    /*
     * -----------------------------------------------------
     * Movement Fallback
     * -----------------------------------------------------
     */

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
            0.90f);

    if (RandomFloat() <=
        result.adaptation_level)
    {
        result.decision =
            decision;

        result.adapted = true;
    }

    return result;
}

/*
 * A decision only counts when the probability roll
 * succeeded. Otherwise the enemy behaves normally.
 */
static AdaptiveDecisionResult ApplyAdaptationRoll(
    AdaptiveDecisionResult result)
{
    if (!result.adapted)
    {
        result.decision =
            ADAPTIVE_DECISION_NONE;
    }

    return result;
}

/*
 * ---------------------------------------------------------
 * Main Adaptive Decision Function
 * ---------------------------------------------------------
 */

AdaptiveDecisionResult AdaptiveAI_Decide(
    AdaptiveAIType type)
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
        return ApplyAdaptationRoll(
            AdaptiveAI_DecideHunter(profile));

    case ADAPTIVE_AI_GUARDIAN:
        return ApplyAdaptationRoll(
            AdaptiveAI_DecideGuardian(profile));

    case ADAPTIVE_AI_ASSASSIN:
        return ApplyAdaptationRoll(
            AdaptiveAI_DecideAssassin(profile));

    case ADAPTIVE_AI_BOSS:
        return ApplyAdaptationRoll(
            AdaptiveAI_DecideBoss(profile));

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
    AdaptiveAIType type)
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
    AdaptiveDecision decision)
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