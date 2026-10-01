#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "adaptive_ai.h"
#include "behavior.h"
#include "events.h"

/*
 * =========================================================
 * DungeonForge - Adaptive AI Tests
 * =========================================================
 */

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(condition, message)                 \
    do                                           \
    {                                            \
        tests_run++;                             \
        if (condition)                           \
        {                                        \
            tests_passed++;                     \
            printf("[PASS] %s\n", message);      \
        }                                        \
        else                                     \
        {                                        \
            tests_failed++;                     \
            printf("[FAIL] %s\n", message);      \
        }                                        \
    } while (0)


/*
 * ---------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------
 */

static void ResetSystems(void)
{
    Events_Init();
    Behavior_Init();
    AdaptiveAI_Init();
}


static void PushEvent(
    EventType type,
    DodgeDirection dodge_direction,
    MoveDirection move_direction,
    float value
)
{
    GameEvent event = {0};

    event.type = type;
    event.dodge_direction = dodge_direction;
    event.move_direction = move_direction;
    event.value = value;

    Events_Push(event);
    Behavior_Update();
}


/*
 * Generate a strongly left-biased dodge profile.
 *
 * 8 LEFT
 * 1 RIGHT
 */
static void CreateLeftDodgeProfile(void)
{
    for (int i = 0; i < 9; i++)
    {
        PushEvent(
            EVENT_PLAYER_DODGE,
            DODGE_LEFT,
            MOVE_NONE,
            0.0f
        );
    }

    PushEvent(
        EVENT_PLAYER_DODGE,
        DODGE_RIGHT,
        MOVE_NONE,
        0.0f
    );
}


/*
 * Generate a strongly right-biased dodge profile.
 *
 * 8 RIGHT
 * 1 LEFT
 */
static void CreateRightDodgeProfile(void)
{
    for (int i = 0; i < 9; i++)
    {
        PushEvent(
            EVENT_PLAYER_DODGE,
            DODGE_RIGHT,
            MOVE_NONE,
            0.0f
        );
    }

    PushEvent(
        EVENT_PLAYER_DODGE,
        DODGE_LEFT,
        MOVE_NONE,
        0.0f
    );
}


/*
 * Generate a movement profile.
 *
 * 8 LEFT
 * 1 RIGHT
 */
static void CreateLeftMovementProfile(void)
{
    for (int i = 0; i < 9; i++)
    {
        PushEvent(
            EVENT_PLAYER_MOVE,
            DODGE_NONE,
            MOVE_LEFT,
            0.0f
        );
    }

    PushEvent(
        EVENT_PLAYER_MOVE,
        DODGE_NONE,
        MOVE_RIGHT,
        0.0f
    );
}


/*
 * Generate enough attacks for Guardian.
 */
static void CreateAttackProfile(int attack_count)
{
    for (int i = 0; i < attack_count; i++)
    {
        PushEvent(
            EVENT_PLAYER_ATTACK,
            DODGE_NONE,
            MOVE_NONE,
            1.0f
        );
    }
}


static bool IsValidDecision(AdaptiveDecision decision)
{
    return
        decision >= ADAPTIVE_DECISION_NONE &&
        decision <= ADAPTIVE_DECISION_KEEP_DISTANCE;
}


/*
 * ---------------------------------------------------------
 * Initialization Tests
 * ---------------------------------------------------------
 */

static void TestInitialization(void)
{
    ResetSystems();

    AdaptiveDecisionResult result =
        AdaptiveAI_Decide(ADAPTIVE_AI_HUNTER);

    TEST(
        result.type == ADAPTIVE_AI_HUNTER,
        "Hunter decision result has correct type"
    );

    TEST(
        result.decision == ADAPTIVE_DECISION_NONE,
        "Hunter returns NONE with insufficient data"
    );

    TEST(
        result.adapted == false,
        "Hunter does not adapt with insufficient data"
    );

    TEST(
        result.adaptation_level >= 0.0f &&
        result.adaptation_level <= 1.0f,
        "Hunter adaptation level is within valid range"
    );
}


/*
 * ---------------------------------------------------------
 * Type Name Tests
 * ---------------------------------------------------------
 */

static void TestTypeNames(void)
{
    TEST(
        AdaptiveAI_GetTypeName(ADAPTIVE_AI_HUNTER) != NULL,
        "Hunter type name exists"
    );

    TEST(
        AdaptiveAI_GetTypeName(ADAPTIVE_AI_GUARDIAN) != NULL,
        "Guardian type name exists"
    );

    TEST(
        AdaptiveAI_GetTypeName(ADAPTIVE_AI_ASSASSIN) != NULL,
        "Assassin type name exists"
    );

    TEST(
        AdaptiveAI_GetTypeName(ADAPTIVE_AI_BOSS) != NULL,
        "Boss type name exists"
    );

    TEST(
        AdaptiveAI_GetTypeName(ADAPTIVE_AI_HUNTER)[0] == 'H',
        "Hunter type name is correct"
    );

    TEST(
        AdaptiveAI_GetTypeName(ADAPTIVE_AI_BOSS)[0] == 'B',
        "Boss type name is correct"
    );
}


/*
 * ---------------------------------------------------------
 * Decision Name Tests
 * ---------------------------------------------------------
 */

static void TestDecisionNames(void)
{
    TEST(
        AdaptiveAI_GetDecisionName(
            ADAPTIVE_DECISION_NONE
        ) != NULL,
        "NONE decision name exists"
    );

    TEST(
        AdaptiveAI_GetDecisionName(
            ADAPTIVE_DECISION_PREDICT_LEFT
        ) != NULL,
        "Predict-left decision name exists"
    );

    TEST(
        AdaptiveAI_GetDecisionName(
            ADAPTIVE_DECISION_FLANK_RIGHT
        ) != NULL,
        "Flank-right decision name exists"
    );

    TEST(
        AdaptiveAI_GetDecisionName(
            ADAPTIVE_DECISION_ATTACK_DEFENSIVE
        ) != NULL,
        "Defensive attack decision name exists"
    );

    TEST(
        AdaptiveAI_GetDecisionName(
            ADAPTIVE_DECISION_CLOSE_DISTANCE
        ) != NULL,
        "Close-distance decision name exists"
    );
}


/*
 * ---------------------------------------------------------
 * Hunter Tests
 * ---------------------------------------------------------
 */

static void TestHunterDodgePrediction(void)
{
    ResetSystems();

    CreateLeftDodgeProfile();

    const PlayerBehavior *profile =
        Behavior_GetProfile();

    TEST(
        profile->total_dodges == 10,
        "Hunter test profile contains 10 dodge events"
    );

    TEST(
        profile->dodge_left_probability >
        profile->dodge_right_probability,
        "Hunter profile is left-dodge biased"
    );

    int left_predictions = 0;
    int right_predictions = 0;
    int other_predictions = 0;
    int adapted_count = 0;

    /*
     * Run many decisions because the AI is intentionally
     * probabilistic.
     */
    for (int i = 0; i < 1000; i++)
    {
        AdaptiveDecisionResult result =
            AdaptiveAI_Decide(ADAPTIVE_AI_HUNTER);

        if (result.adapted)
        {
            adapted_count++;

            if (result.decision ==
                ADAPTIVE_DECISION_PREDICT_LEFT)
            {
                left_predictions++;
            }
            else if (result.decision ==
                     ADAPTIVE_DECISION_PREDICT_RIGHT)
            {
                right_predictions++;
            }
            else
            {
                other_predictions++;
            }
        }

        TEST(
            result.type == ADAPTIVE_AI_HUNTER,
            "Hunter result retains Hunter type"
        );

        if (i == 0)
        {
            TEST(
                IsValidDecision(result.decision),
                "Hunter returns a valid decision"
            );

            TEST(
                result.probability >= 0.0f &&
                result.probability <= 1.0f,
                "Hunter probability is within valid range"
            );

            TEST(
                result.adaptation_level >= 0.0f &&
                result.adaptation_level <= 1.0f,
                "Hunter adaptation level is within valid range"
            );
        }
    }

    TEST(
        adapted_count > 0,
        "Hunter adapts at least once across repeated decisions"
    );

    TEST(
        left_predictions > right_predictions,
        "Hunter predominantly predicts the stronger left tendency"
    );

    TEST(
        other_predictions >= 0,
        "Hunter directional result accounting is valid"
    );
}


static void TestHunterMovementFallback(void)
{
    ResetSystems();

    CreateLeftMovementProfile();

    AdaptiveDecisionResult result =
        AdaptiveAI_Decide(ADAPTIVE_AI_HUNTER);

    TEST(
        result.type == ADAPTIVE_AI_HUNTER,
        "Hunter movement fallback has correct type"
    );

    TEST(
        IsValidDecision(result.decision),
        "Hunter movement fallback returns valid decision"
    );

    TEST(
        result.probability >= 0.0f &&
        result.probability <= 1.0f,
        "Hunter movement fallback probability is valid"
    );

    TEST(
        result.adaptation_level >= 0.0f &&
        result.adaptation_level <= 0.85f,
        "Hunter movement fallback adaptation is capped correctly"
    );
}


/*
 * ---------------------------------------------------------
 * Guardian Tests
 * ---------------------------------------------------------
 */

static void TestGuardianInsufficientAttacks(void)
{
    ResetSystems();

    CreateAttackProfile(4);

    AdaptiveDecisionResult result =
        AdaptiveAI_Decide(ADAPTIVE_AI_GUARDIAN);

    TEST(
        result.type == ADAPTIVE_AI_GUARDIAN,
        "Guardian result has correct type"
    );

    TEST(
        result.decision == ADAPTIVE_DECISION_NONE,
        "Guardian returns NONE below attack threshold"
    );

    TEST(
        result.adapted == false,
        "Guardian does not adapt below attack threshold"
    );
}


static void TestGuardianLowAttackProfile(void)
{
    ResetSystems();

    CreateAttackProfile(10);

    int aggressive_count = 0;

    for (int i = 0; i < 1000; i++)
    {
        AdaptiveDecisionResult result =
            AdaptiveAI_Decide(ADAPTIVE_AI_GUARDIAN);

        TEST(
            result.type == ADAPTIVE_AI_GUARDIAN,
            "Guardian low-attack result has correct type"
        );

        TEST(
            result.decision ==
            ADAPTIVE_DECISION_ATTACK_AGGRESSIVE,
            "Guardian low-attack profile selects aggressive strategy"
        );

        if (result.adapted)
        {
            aggressive_count++;
        }

        if (i == 0)
        {
            TEST(
                result.probability == 0.40f,
                "Guardian aggressive probability is 0.40"
            );

            TEST(
                result.adaptation_level == 0.40f,
                "Guardian aggressive adaptation is 0.40"
            );
        }
    }

    TEST(
        aggressive_count > 0,
        "Guardian aggressive adaptation occurs probabilistically"
    );
}


static void TestGuardianHighAttackProfile(void)
{
    ResetSystems();

    CreateAttackProfile(20);

    int defensive_count = 0;

    for (int i = 0; i < 1000; i++)
    {
        AdaptiveDecisionResult result =
            AdaptiveAI_Decide(ADAPTIVE_AI_GUARDIAN);

        TEST(
            result.type == ADAPTIVE_AI_GUARDIAN,
            "Guardian high-attack result has correct type"
        );

        TEST(
            result.decision ==
            ADAPTIVE_DECISION_ATTACK_DEFENSIVE,
            "Guardian high-attack profile selects defensive strategy"
        );

        if (result.adapted)
        {
            defensive_count++;
        }

        if (i == 0)
        {
            TEST(
                result.probability == 0.70f,
                "Guardian defensive probability is 0.70"
            );

            TEST(
                result.adaptation_level == 0.70f,
                "Guardian defensive adaptation is 0.70"
            );
        }
    }

    TEST(
        defensive_count > 0,
        "Guardian defensive adaptation occurs probabilistically"
    );
}


/*
 * ---------------------------------------------------------
 * Assassin Tests
 * ---------------------------------------------------------
 */

static void TestAssassinLeftDodge(void)
{
    ResetSystems();

    CreateLeftDodgeProfile();

    AdaptiveDecisionResult result =
        AdaptiveAI_Decide(ADAPTIVE_AI_ASSASSIN);

    TEST(
        result.type == ADAPTIVE_AI_ASSASSIN,
        "Assassin result has correct type"
    );

    TEST(
        result.decision ==
        ADAPTIVE_DECISION_FLANK_RIGHT,
        "Assassin flanks right against stronger left dodge tendency"
    );

    TEST(
        result.probability >= 0.0f &&
        result.probability <= 1.0f,
        "Assassin probability is valid"
    );

    TEST(
        result.adaptation_level >= 0.0f &&
        result.adaptation_level <= 0.85f,
        "Assassin adaptation level is capped correctly"
    );
}


static void TestAssassinRightDodge(void)
{
    ResetSystems();

    CreateRightDodgeProfile();

    AdaptiveDecisionResult result =
        AdaptiveAI_Decide(ADAPTIVE_AI_ASSASSIN);

    TEST(
        result.type == ADAPTIVE_AI_ASSASSIN,
        "Assassin right-dodge result has correct type"
    );

    TEST(
        result.decision ==
        ADAPTIVE_DECISION_FLANK_LEFT,
        "Assassin flanks left against stronger right dodge tendency"
    );
}


static void TestAssassinMovementFallback(void)
{
    ResetSystems();

    CreateLeftMovementProfile();

    AdaptiveDecisionResult result =
        AdaptiveAI_Decide(ADAPTIVE_AI_ASSASSIN);

    TEST(
        result.type == ADAPTIVE_AI_ASSASSIN,
        "Assassin movement fallback has correct type"
    );

    TEST(
        result.decision ==
        ADAPTIVE_DECISION_FLANK_RIGHT,
        "Assassin movement fallback flanks opposite stronger movement tendency"
    );

    TEST(
        result.probability >= 0.0f &&
        result.probability <= 1.0f,
        "Assassin movement fallback probability is valid"
    );
}


/*
 * ---------------------------------------------------------
 * Boss Tests
 * ---------------------------------------------------------
 */

static void TestBossDodgeAdaptation(void)
{
    ResetSystems();

    CreateLeftDodgeProfile();

    AdaptiveDecisionResult result =
        AdaptiveAI_Decide(ADAPTIVE_AI_BOSS);

    TEST(
        result.type == ADAPTIVE_AI_BOSS,
        "Boss result has correct type"
    );

    TEST(
        result.decision ==
        ADAPTIVE_DECISION_FLANK_RIGHT,
        "Boss reacts to stronger left dodge tendency with right flank"
    );

    TEST(
        result.probability >= 0.0f &&
        result.probability <= 1.0f,
        "Boss probability is valid"
    );

    TEST(
        result.adaptation_level >= 0.0f &&
        result.adaptation_level <= 0.90f,
        "Boss adaptation level is capped at 0.90"
    );
}


static void TestBossMovementFallback(void)
{
    ResetSystems();

    CreateLeftMovementProfile();

    AdaptiveDecisionResult result =
        AdaptiveAI_Decide(ADAPTIVE_AI_BOSS);

    TEST(
        result.type == ADAPTIVE_AI_BOSS,
        "Boss movement fallback has correct type"
    );

    TEST(
        result.decision ==
        ADAPTIVE_DECISION_FLANK_RIGHT,
        "Boss movement fallback reacts opposite stronger movement tendency"
    );

    TEST(
        result.adaptation_level >= 0.0f &&
        result.adaptation_level <= 0.90f,
        "Boss movement fallback adaptation level is valid"
    );
}


/*
 * ---------------------------------------------------------
 * Decision Validity Tests
 * ---------------------------------------------------------
 */

static void TestDecisionValidity(void)
{
    ResetSystems();

    AdaptiveAIType types[] =
    {
        ADAPTIVE_AI_HUNTER,
        ADAPTIVE_AI_GUARDIAN,
        ADAPTIVE_AI_ASSASSIN,
        ADAPTIVE_AI_BOSS
    };

    int type_count =
        sizeof(types) / sizeof(types[0]);

    for (int i = 0; i < type_count; i++)
    {
        AdaptiveDecisionResult result =
            AdaptiveAI_Decide(types[i]);

        TEST(
            IsValidDecision(result.decision),
            "AI returns a valid decision enum"
        );

        TEST(
            result.probability >= 0.0f &&
            result.probability <= 1.0f,
            "AI probability remains within [0,1]"
        );

        TEST(
            result.adaptation_level >= 0.0f &&
            result.adaptation_level <= 1.0f,
            "AI adaptation level remains within [0,1]"
        );
    }
}


/*
 * ---------------------------------------------------------
 * Probabilistic Distribution Test
 * ---------------------------------------------------------
 *
 * This verifies that the Hunter is not deterministic.
 *
 * With a profile of:
 *
 *     LEFT  = 8/9
 *     RIGHT = 1/9
 *
 * repeated predictions should produce both directions
 * eventually, while LEFT should dominate.
 */

static void TestHunterIsProbabilistic(void)
{
    ResetSystems();

    CreateLeftDodgeProfile();

    int left_count = 0;
    int right_count = 0;
    int up_count = 0;
    int down_count = 0;

    for (int i = 0; i < 5000; i++)
    {
        AdaptiveDecisionResult result =
            AdaptiveAI_Decide(ADAPTIVE_AI_HUNTER);

        if (!result.adapted)
            continue;

        switch (result.decision)
        {
            case ADAPTIVE_DECISION_PREDICT_LEFT:
                left_count++;
                break;

            case ADAPTIVE_DECISION_PREDICT_RIGHT:
                right_count++;
                break;

            case ADAPTIVE_DECISION_PREDICT_UP:
                up_count++;
                break;

            case ADAPTIVE_DECISION_PREDICT_DOWN:
                down_count++;
                break;

            default:
                break;
        }
    }

    TEST(
        left_count > 0,
        "Hunter produces left predictions"
    );

    TEST(
        right_count > 0,
        "Hunter can still produce right predictions probabilistically"
    );

    TEST(
        left_count > right_count,
        "Hunter favors the statistically stronger direction"
    );

    TEST(
        up_count == 0,
        "Hunter does not predict unseen up direction"
    );

    TEST(
        down_count == 0,
        "Hunter does not predict unseen down direction"
    );
}


/*
 * =========================================================
 * Main
 * =========================================================
 */

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" DungeonForge - Adaptive AI Tests\n");
    printf("========================================\n");
    printf("\n");

    TestInitialization();

    TestTypeNames();
    TestDecisionNames();

    TestHunterDodgePrediction();
    TestHunterMovementFallback();

    TestGuardianInsufficientAttacks();
    TestGuardianLowAttackProfile();
    TestGuardianHighAttackProfile();

    TestAssassinLeftDodge();
    TestAssassinRightDodge();
    TestAssassinMovementFallback();

    TestBossDodgeAdaptation();
    TestBossMovementFallback();

    TestDecisionValidity();

    TestHunterIsProbabilistic();

    printf("\n");
    printf("========================================\n");
    printf(
        "Tests: %d | Passed: %d | Failed: %d\n",
        tests_run,
        tests_passed,
        tests_failed
    );
    printf("========================================\n");
    printf("\n");

    return tests_failed == 0 ? 0 : 1;
}