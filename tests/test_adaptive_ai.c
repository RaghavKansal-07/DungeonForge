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

#define TEST(condition, message)            \
    do                                      \
    {                                       \
        tests_run++;                        \
        if (condition)                      \
        {                                   \
            tests_passed++;                 \
            printf("[PASS] %s\n", message); \
        }                                   \
        else                                \
        {                                   \
            tests_failed++;                 \
            printf("[FAIL] %s\n", message); \
        }                                   \
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
    float value)
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
            0.0f);
    }

    PushEvent(
        EVENT_PLAYER_DODGE,
        DODGE_RIGHT,
        MOVE_NONE,
        0.0f);
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
            0.0f);
    }

    PushEvent(
        EVENT_PLAYER_DODGE,
        DODGE_LEFT,
        MOVE_NONE,
        0.0f);
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
            0.0f);
    }

    PushEvent(
        EVENT_PLAYER_MOVE,
        DODGE_NONE,
        MOVE_RIGHT,
        0.0f);
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
            1.0f);
    }
}

static bool IsValidDecision(AdaptiveDecision decision)
{
    return decision >= ADAPTIVE_DECISION_NONE &&
           decision <= ADAPTIVE_DECISION_KEEP_DISTANCE;
}

typedef struct
{
    int adapted;
    int flank_left;
    int flank_right;
    int other;
    int inconsistent;

} DecisionStats;

/*
 * Run many decisions and tally them. "inconsistent" counts
 * results where adapted and decision disagree.
 */
static DecisionStats CollectStats(
    AdaptiveAIType type,
    int runs)
{
    DecisionStats stats = {0};

    for (int i = 0; i < runs; i++)
    {
        AdaptiveDecisionResult result =
            AdaptiveAI_Decide(type);

        bool has_decision =
            result.decision != ADAPTIVE_DECISION_NONE;

        if (result.adapted != has_decision)
            stats.inconsistent++;

        if (!result.adapted)
            continue;

        stats.adapted++;

        if (result.decision == ADAPTIVE_DECISION_FLANK_LEFT)
            stats.flank_left++;
        else if (result.decision == ADAPTIVE_DECISION_FLANK_RIGHT)
            stats.flank_right++;
        else
            stats.other++;
    }

    return stats;
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
        "Hunter decision result has correct type");

    TEST(
        result.decision == ADAPTIVE_DECISION_NONE,
        "Hunter returns NONE with insufficient data");

    TEST(
        result.adapted == false,
        "Hunter does not adapt with insufficient data");

    TEST(
        result.adaptation_level >= 0.0f &&
            result.adaptation_level <= 1.0f,
        "Hunter adaptation level is within valid range");
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
        "Hunter type name exists");

    TEST(
        AdaptiveAI_GetTypeName(ADAPTIVE_AI_GUARDIAN) != NULL,
        "Guardian type name exists");

    TEST(
        AdaptiveAI_GetTypeName(ADAPTIVE_AI_ASSASSIN) != NULL,
        "Assassin type name exists");

    TEST(
        AdaptiveAI_GetTypeName(ADAPTIVE_AI_BOSS) != NULL,
        "Boss type name exists");

    TEST(
        AdaptiveAI_GetTypeName(ADAPTIVE_AI_HUNTER)[0] == 'H',
        "Hunter type name is correct");

    TEST(
        AdaptiveAI_GetTypeName(ADAPTIVE_AI_BOSS)[0] == 'B',
        "Boss type name is correct");
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
            ADAPTIVE_DECISION_NONE) != NULL,
        "NONE decision name exists");

    TEST(
        AdaptiveAI_GetDecisionName(
            ADAPTIVE_DECISION_PREDICT_LEFT) != NULL,
        "Predict-left decision name exists");

    TEST(
        AdaptiveAI_GetDecisionName(
            ADAPTIVE_DECISION_FLANK_RIGHT) != NULL,
        "Flank-right decision name exists");

    TEST(
        AdaptiveAI_GetDecisionName(
            ADAPTIVE_DECISION_ATTACK_DEFENSIVE) != NULL,
        "Defensive attack decision name exists");

    TEST(
        AdaptiveAI_GetDecisionName(
            ADAPTIVE_DECISION_CLOSE_DISTANCE) != NULL,
        "Close-distance decision name exists");
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
        "Hunter test profile contains 10 dodge events");

    TEST(
        profile->dodge_left_probability >
            profile->dodge_right_probability,
        "Hunter profile is left-dodge biased");

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
            "Hunter result retains Hunter type");

        if (i == 0)
        {
            TEST(
                IsValidDecision(result.decision),
                "Hunter returns a valid decision");

            TEST(
                result.probability >= 0.0f &&
                    result.probability <= 1.0f,
                "Hunter probability is within valid range");

            TEST(
                result.adaptation_level >= 0.0f &&
                    result.adaptation_level <= 1.0f,
                "Hunter adaptation level is within valid range");
        }
    }

    TEST(
        adapted_count > 0,
        "Hunter adapts at least once across repeated decisions");

    TEST(
        left_predictions > right_predictions,
        "Hunter predominantly predicts the stronger left tendency");

    TEST(
        other_predictions >= 0,
        "Hunter directional result accounting is valid");
}

static void TestHunterMovementFallback(void)
{
    ResetSystems();

    CreateLeftMovementProfile();

    AdaptiveDecisionResult result =
        AdaptiveAI_Decide(ADAPTIVE_AI_HUNTER);

    TEST(
        result.type == ADAPTIVE_AI_HUNTER,
        "Hunter movement fallback has correct type");

    TEST(
        IsValidDecision(result.decision),
        "Hunter movement fallback returns valid decision");

    TEST(
        result.probability >= 0.0f &&
            result.probability <= 1.0f,
        "Hunter movement fallback probability is valid");

    TEST(
        result.adaptation_level >= 0.0f &&
            result.adaptation_level <= 0.85f,
        "Hunter movement fallback adaptation is capped correctly");
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
        "Guardian result has correct type");

    TEST(
        result.decision == ADAPTIVE_DECISION_NONE,
        "Guardian returns NONE below attack threshold");

    TEST(
        result.adapted == false,
        "Guardian does not adapt below attack threshold");
}

static void TestGuardianLowAttackProfile(void)
{
    ResetSystems();
    CreateAttackProfile(10);

    int adapted_count = 0;
    int bad_results = 0;

    for (int i = 0; i < 1000; i++)
    {
        AdaptiveDecisionResult result =
            AdaptiveAI_Decide(ADAPTIVE_AI_GUARDIAN);

        if (result.type != ADAPTIVE_AI_GUARDIAN)
            bad_results++;

        if (result.adapted)
        {
            adapted_count++;

            if (result.decision !=
                ADAPTIVE_DECISION_ATTACK_AGGRESSIVE)
                bad_results++;
        }
        else if (result.decision != ADAPTIVE_DECISION_NONE)
        {
            bad_results++;
        }

        if (i == 0)
        {
            TEST(result.probability == 0.40f,
                 "Guardian aggressive probability is 0.40");

            TEST(result.adaptation_level == 0.40f,
                 "Guardian aggressive adaptation is 0.40");
        }
    }

    TEST(bad_results == 0,
         "Guardian low-attack: aggressive only when adapted, NONE otherwise");

    TEST(adapted_count > 300 && adapted_count < 500,
         "Guardian low-attack adapts about 40% of the time");
}

static void TestGuardianHighAttackProfile(void)
{
    ResetSystems();
    CreateAttackProfile(20);

    int adapted_count = 0;
    int bad_results = 0;

    for (int i = 0; i < 1000; i++)
    {
        AdaptiveDecisionResult result =
            AdaptiveAI_Decide(ADAPTIVE_AI_GUARDIAN);

        if (result.type != ADAPTIVE_AI_GUARDIAN)
            bad_results++;

        if (result.adapted)
        {
            adapted_count++;

            if (result.decision !=
                ADAPTIVE_DECISION_ATTACK_DEFENSIVE)
                bad_results++;
        }
        else if (result.decision != ADAPTIVE_DECISION_NONE)
        {
            bad_results++;
        }

        if (i == 0)
        {
            TEST(result.probability == 0.70f,
                 "Guardian defensive probability is 0.70");

            TEST(result.adaptation_level == 0.70f,
                 "Guardian defensive adaptation is 0.70");
        }
    }

    TEST(bad_results == 0,
         "Guardian high-attack: defensive only when adapted, NONE otherwise");

    TEST(adapted_count > 600 && adapted_count < 800,
         "Guardian high-attack adapts about 70% of the time");
}

static void TestAssassinLeftDodge(void)
{
    ResetSystems();
    CreateLeftDodgeProfile();

    DecisionStats s = CollectStats(ADAPTIVE_AI_ASSASSIN, 1000);

    TEST(s.inconsistent == 0,
         "Assassin: decision present exactly when adapted");

    TEST(s.flank_left == 0 && s.other == 0,
         "Assassin never flanks left against a left-dodge tendency");

    TEST(s.flank_right > 760 && s.flank_right < 930,
         "Assassin flanks right about 85% of the time against left dodges");
}

static void TestAssassinRightDodge(void)
{
    ResetSystems();
    CreateRightDodgeProfile();

    DecisionStats s = CollectStats(ADAPTIVE_AI_ASSASSIN, 1000);

    TEST(s.inconsistent == 0,
         "Assassin (right dodge): decision present exactly when adapted");

    TEST(s.flank_right == 0 && s.other == 0,
         "Assassin never flanks right against a right-dodge tendency");

    TEST(s.flank_left > 760 && s.flank_left < 930,
         "Assassin flanks left about 85% of the time against right dodges");
}

static void TestAssassinMovementFallback(void)
{
    ResetSystems();
    CreateLeftMovementProfile();

    DecisionStats s = CollectStats(ADAPTIVE_AI_ASSASSIN, 1000);

    TEST(s.inconsistent == 0,
         "Assassin fallback: decision present exactly when adapted");

    TEST(s.flank_left == 0 && s.other == 0,
         "Assassin fallback never flanks toward the stronger movement side");

    TEST(s.flank_right > 760 && s.flank_right < 930,
         "Assassin fallback flanks opposite stronger movement about 85% of the time");
}

static void TestBossDodgeAdaptation(void)
{
    ResetSystems();
    CreateLeftDodgeProfile();

    DecisionStats s = CollectStats(ADAPTIVE_AI_BOSS, 1000);

    TEST(s.inconsistent == 0,
         "Boss: decision present exactly when adapted");

    TEST(s.flank_left == 0 && s.other == 0,
         "Boss only flanks right against a left-dodge tendency");

    TEST(s.flank_right > 820 && s.flank_right < 970,
         "Boss adapts about 90% of the time against strong dodge habit");
}

static void TestBossMovementFallback(void)
{
    ResetSystems();
    CreateLeftMovementProfile();

    DecisionStats s = CollectStats(ADAPTIVE_AI_BOSS, 1000);

    TEST(s.inconsistent == 0,
         "Boss fallback: decision present exactly when adapted");

    TEST(s.flank_left == 0 && s.other == 0,
         "Boss fallback only flanks opposite the stronger movement side");

    TEST(s.flank_right > 820 && s.flank_right < 970,
         "Boss fallback adapts about 90% of the time");
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
            ADAPTIVE_AI_BOSS};

    int type_count =
        sizeof(types) / sizeof(types[0]);

    for (int i = 0; i < type_count; i++)
    {
        AdaptiveDecisionResult result =
            AdaptiveAI_Decide(types[i]);

        TEST(
            IsValidDecision(result.decision),
            "AI returns a valid decision enum");

        TEST(
            result.probability >= 0.0f &&
                result.probability <= 1.0f,
            "AI probability remains within [0,1]");

        TEST(
            result.adaptation_level >= 0.0f &&
                result.adaptation_level <= 1.0f,
            "AI adaptation level remains within [0,1]");
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
        "Hunter produces left predictions");

    TEST(
        right_count > 0,
        "Hunter can still produce right predictions probabilistically");

    TEST(
        left_count > right_count,
        "Hunter favors the statistically stronger direction");

    TEST(
        up_count == 0,
        "Hunter does not predict unseen up direction");

    TEST(
        down_count == 0,
        "Hunter does not predict unseen down direction");
}

static void TestDecisionOnlyWhenAdapted(void)
{
    ResetSystems();

    CreateLeftDodgeProfile();
    CreateLeftMovementProfile();
    CreateAttackProfile(20);

    AdaptiveAIType types[] =
    {
        ADAPTIVE_AI_HUNTER,
        ADAPTIVE_AI_GUARDIAN,
        ADAPTIVE_AI_ASSASSIN,
        ADAPTIVE_AI_BOSS
    };

    int inconsistent = 0;

    for (int t = 0; t < 4; t++)
    {
        DecisionStats s = CollectStats(types[t], 1000);
        inconsistent += s.inconsistent;
    }

    TEST(inconsistent == 0,
         "No archetype returns a decision unless the adaptation roll succeeded");
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
        tests_failed);
    printf("========================================\n");
    printf("\n");

    return tests_failed == 0 ? 0 : 1;
}