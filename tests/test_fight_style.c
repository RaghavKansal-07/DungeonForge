#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "adaptive_ai.h"
#include "behavior.h"
#include "events.h"

static int tests_run = 0;
static int tests_passed = 0;

static void Test_Assert(
    bool condition,
    const char *test_name)
{
    tests_run++;

    if (condition)
    {
        tests_passed++;

        printf("[PASS] %s\n", test_name);
    }
    else
    {
        printf("[FAIL] %s\n", test_name);
    }
}

static void ResetSystems(void)
{
    Events_Init();
    Behavior_Init();
    AdaptiveAI_Init();
    AdaptiveAI_Seed(4242u);
}

static void PushAttack(float distance)
{
    GameEvent event = {0};

    event.type = EVENT_PLAYER_ATTACK;
    event.value = distance;

    Events_Push(event);
    Behavior_Update();
}

static void PushDodge(DodgeDirection direction)
{
    GameEvent event = {0};

    event.type = EVENT_PLAYER_DODGE;
    event.dodge_direction = direction;

    Events_Push(event);
    Behavior_Update();
}

static void PushAttacks(int count, float distance)
{
    for (int i = 0; i < count; i++)
        PushAttack(distance);
}

typedef struct
{
    int adapted;
    int keep_distance;
    int flank_right;
    int other;
    int inconsistent;

} Stats;

static Stats Collect(AdaptiveAIType type, int runs)
{
    Stats stats = {0};

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

        if (result.decision == ADAPTIVE_DECISION_KEEP_DISTANCE)
            stats.keep_distance++;
        else if (result.decision == ADAPTIVE_DECISION_FLANK_RIGHT)
            stats.flank_right++;
        else
            stats.other++;
    }

    return stats;
}

/*
 * ---------------------------------------------------------
 * Recording attack distance
 * ---------------------------------------------------------
 */

static void Test_Bands(void)
{
    ResetSystems();

    PushAttack(30.0f);
    PushAttack(60.0f);
    PushAttack(70.0f);
    PushAttack(-1.0f);

    const PlayerBehavior *p = Behavior_GetProfile();

    Test_Assert(
        p->total_attacks == 4 &&
            p->attacks_measured == 3,
        "Attacks with no enemy nearby are counted but not measured");

    Test_Assert(
        p->close_attacks == 1 &&
            p->mid_attacks == 1 &&
            p->far_attacks == 1,
        "Attacks are sorted into close, mid and far");

    ResetSystems();

    PushAttack(0.0f);
    PushAttack(52.0f);
    PushAttack(52.1f);
    PushAttack(66.0f);
    PushAttack(66.1f);

    p = Behavior_GetProfile();

    Test_Assert(
        p->close_attacks == 2 &&
            p->mid_attacks == 2 &&
            p->far_attacks == 1,
        "The band edges fall where expected (52 and 66 pixels)");

    ResetSystems();

    PushAttack(NAN);

    Test_Assert(
        Behavior_GetProfile()->attacks_measured == 0,
        "An invalid distance is not measured");

    ResetSystems();

    PushAttacks(3, 30.0f);
    PushAttacks(1, 70.0f);

    Test_Assert(
        fabsf(Behavior_GetProfile()->close_attack_ratio - 0.75f) <
            0.0001f,
        "The close-attack ratio is derived correctly");
}

static void Test_FightStyle(void)
{
    ResetSystems();

    PushAttacks(5, 30.0f);

    Test_Assert(
        Behavior_GetFightStyle() == FIGHT_STYLE_UNKNOWN &&
            Behavior_GetFightStyleShare() == 0.0f,
        "The fight style stays unknown below 6 measured attacks");

    PushAttacks(3, 30.0f);

    Test_Assert(
        Behavior_GetFightStyle() == FIGHT_STYLE_CLOSE,
        "Mostly close attacks give the close fight style");

    ResetSystems();
    PushAttacks(8, 60.0f);

    Test_Assert(
        Behavior_GetFightStyle() == FIGHT_STYLE_MID,
        "Mostly mid-range attacks give the mid fight style");

    ResetSystems();
    PushAttacks(8, 72.0f);

    Test_Assert(
        Behavior_GetFightStyle() == FIGHT_STYLE_FAR,
        "Mostly long-reach attacks give the far fight style");

    ResetSystems();
    PushAttacks(8, 30.0f);
    PushAttacks(2, 72.0f);

    Test_Assert(
        fabsf(Behavior_GetFightStyleShare() - 0.8f) < 0.0001f,
        "The fight style share is the dominant band's fraction");
}

static void Test_SaveAndLoad(void)
{
    ResetSystems();

    PushAttacks(7, 30.0f);
    PushAttacks(3, 70.0f);

    PlayerBehavior saved = *Behavior_GetProfile();

    Behavior_Init();

    Test_Assert(
        Behavior_Restore(&saved) &&
            Behavior_GetProfile()->close_attacks == 7 &&
            Behavior_GetProfile()->far_attacks == 3 &&
            fabsf(Behavior_GetProfile()->close_attack_ratio - 0.7f) <
                0.0001f,
        "Attack-distance statistics survive a restore");

    PlayerBehavior bad = saved;
    bad.close_attacks = 99;

    Test_Assert(
        !Behavior_Restore(&bad),
        "A save whose bands do not add up is rejected");

    bad = saved;
    bad.attacks_measured = saved.total_attacks + 5;

    Test_Assert(
        !Behavior_Restore(&bad),
        "A save with more measured attacks than attacks is rejected");

    bad = saved;
    bad.far_attacks = -1;

    Test_Assert(
        !Behavior_Restore(&bad),
        "A save with a negative band count is rejected");

    Test_Assert(
        Behavior_GetProfile()->close_attacks == 7,
        "Rejected saves leave the profile unchanged");
}

/*
 * ---------------------------------------------------------
 * Boss reaction
 * ---------------------------------------------------------
 */

static void Test_BossKeepsDistance(void)
{
    ResetSystems();

    PushAttacks(10, 30.0f);

    Stats s = Collect(ADAPTIVE_AI_BOSS, 1000);

    Test_Assert(s.inconsistent == 0,
                "Boss vs brawler: a decision exists exactly when adapted");

    Test_Assert(s.keep_distance == s.adapted && s.adapted > 0,
                "The boss keeps its distance against a close-range fighter");

    Test_Assert(s.adapted > 820 && s.adapted < 970,
                "The boss adapts about 90% of the time against a brawler");
}

static void Test_BossSignalChoice(void)
{
    /*
     * A strong dodge habit (90%) beats a weaker brawler signal
     * (60% close attacks): the boss flanks instead.
     */
    ResetSystems();

    for (int i = 0; i < 9; i++)
        PushDodge(DODGE_LEFT);

    PushDodge(DODGE_RIGHT);

    PushAttacks(6, 30.0f);
    PushAttacks(4, 72.0f);

    Stats s = Collect(ADAPTIVE_AI_BOSS, 1000);

    Test_Assert(
        s.keep_distance == 0 &&
            s.flank_right == s.adapted &&
            s.adapted > 820,
        "A stronger dodge habit beats a weaker brawler signal");

    /*
     * Too few measured attacks: the brawler signal is ignored.
     */
    ResetSystems();

    for (int i = 0; i < 9; i++)
        PushDodge(DODGE_LEFT);

    PushDodge(DODGE_RIGHT);

    PushAttacks(5, 30.0f);

    s = Collect(ADAPTIVE_AI_BOSS, 1000);

    Test_Assert(
        s.keep_distance == 0 && s.flank_right > 800,
        "The boss ignores the attack-distance signal below 6 samples");

    /*
     * Mid and far fighters do not trigger keep-distance.
     */
    ResetSystems();

    PushAttacks(10, 72.0f);

    s = Collect(ADAPTIVE_AI_BOSS, 1000);

    Test_Assert(
        s.keep_distance == 0,
        "A long-reach fighter does not make the boss keep its distance");
}

static void Test_OtherEnemiesIgnoreDistance(void)
{
    ResetSystems();

    PushAttacks(10, 30.0f);

    Stats hunter = Collect(ADAPTIVE_AI_HUNTER, 500);
    Stats assassin = Collect(ADAPTIVE_AI_ASSASSIN, 500);

    Test_Assert(
        hunter.adapted == 0 && assassin.adapted == 0,
        "Hunter and Assassin do not react to attack distance");
}

static void Test_BossRegression(void)
{
    ResetSystems();

    for (int i = 0; i < 9; i++)
        PushDodge(DODGE_LEFT);

    PushDodge(DODGE_RIGHT);

    Stats s = Collect(ADAPTIVE_AI_BOSS, 1000);

    Test_Assert(
        s.inconsistent == 0 &&
            s.flank_right == s.adapted &&
            s.adapted > 820 && s.adapted < 970,
        "The boss still flanks right against a left-dodge habit");
}

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" DungeonForge - Fight Style Tests\n");
    printf("========================================\n\n");

    Test_Bands();

    Test_FightStyle();

    Test_SaveAndLoad();

    Test_BossKeepsDistance();

    Test_BossSignalChoice();

    Test_OtherEnemiesIgnoreDistance();

    Test_BossRegression();

    printf("\n========================================\n");
    printf(
        "Tests: %d | Passed: %d | Failed: %d\n",
        tests_run,
        tests_passed,
        tests_run - tests_passed);
    printf("========================================\n\n");

    return tests_run == tests_passed ? 0 : 1;
}