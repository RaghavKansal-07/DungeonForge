#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#include "behavior.h"
#include "events.h"


/*
 * ---------------------------------------------------------
 * TEST HELPERS
 * ---------------------------------------------------------
 */

static int tests_run = 0;
static int tests_passed = 0;


static void Test_Assert(
    bool condition,
    const char *test_name
)
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


static bool Float_Equals(
    float a,
    float b
)
{
    const float epsilon = 0.0001f;

    return fabsf(a - b) < epsilon;
}


static GameEvent CreateDodgeEvent(
    DodgeDirection direction
)
{
    GameEvent event = {0};

    event.type = EVENT_PLAYER_DODGE;
    event.dodge_direction = direction;

    return event;
}


static GameEvent CreateMoveEvent(
    MoveDirection direction
)
{
    GameEvent event = {0};

    event.type = EVENT_PLAYER_MOVE;
    event.move_direction = direction;

    return event;
}


static GameEvent CreateDamageEvent(
    float damage
)
{
    GameEvent event = {0};

    event.type = EVENT_PLAYER_DAMAGE;
    event.value = damage;

    return event;
}


static GameEvent CreateHealEvent(
    float healing
)
{
    GameEvent event = {0};

    event.type = EVENT_PLAYER_HEAL;
    event.value = healing;

    return event;
}


/*
 * ---------------------------------------------------------
 * TEST 1
 * Empty behavior profile
 * ---------------------------------------------------------
 */

static void Test_EmptyProfile(void)
{
    Events_Init();
    Behavior_Init();

    Behavior_Update();

    const PlayerBehavior *profile =
        Behavior_GetProfile();

    Test_Assert(
        profile->initialized == true,
        "Behavior initializes correctly"
    );

    Test_Assert(
        profile->total_movements == 0,
        "Initial movement count is zero"
    );

    Test_Assert(
        profile->total_dodges == 0,
        "Initial dodge count is zero"
    );

    Test_Assert(
        Float_Equals(
            Behavior_GetDodgeLeftProbability(),
            0.0f
        ),
        "Initial dodge-left probability is zero"
    );

    Test_Assert(
        Float_Equals(
            Behavior_GetDodgeRightProbability(),
            0.0f
        ),
        "Initial dodge-right probability is zero"
    );

    Test_Assert(
        Behavior_HasEnoughData() == false,
        "Empty profile does not have enough data"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 2
 * Dodge probabilities
 * ---------------------------------------------------------
 *
 * 8 LEFT
 * 2 RIGHT
 *
 * Expected:
 *
 * LEFT  = 0.8
 * RIGHT = 0.2
 */

static void Test_DodgeProbabilities(void)
{
    Events_Init();
    Behavior_Init();

    for (int i = 0; i < 8; i++)
    {
        Events_Push(
            CreateDodgeEvent(DODGE_LEFT)
        );
    }

    for (int i = 0; i < 2; i++)
    {
        Events_Push(
            CreateDodgeEvent(DODGE_RIGHT)
        );
    }

    Behavior_Update();

    const PlayerBehavior *profile =
        Behavior_GetProfile();

    Test_Assert(
        profile->total_dodges == 10,
        "Dodge events are counted"
    );

    Test_Assert(
        profile->dodge_left_count == 8,
        "Left dodge count is correct"
    );

    Test_Assert(
        profile->dodge_right_count == 2,
        "Right dodge count is correct"
    );

    Test_Assert(
        Float_Equals(
            Behavior_GetDodgeLeftProbability(),
            0.8f
        ),
        "Left dodge probability is correct"
    );

    Test_Assert(
        Float_Equals(
            Behavior_GetDodgeRightProbability(),
            0.2f
        ),
        "Right dodge probability is correct"
    );

    Test_Assert(
        Float_Equals(
            Behavior_GetDodgeUpProbability(),
            0.0f
        ),
        "Up dodge probability is zero"
    );

    Test_Assert(
        Float_Equals(
            Behavior_GetDodgeDownProbability(),
            0.0f
        ),
        "Down dodge probability is zero"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 3
 * All four dodge directions
 * ---------------------------------------------------------
 */

static void Test_AllDodgeDirections(void)
{
    Events_Init();
    Behavior_Init();

    for (int i = 0; i < 4; i++)
    {
        Events_Push(
            CreateDodgeEvent(DODGE_LEFT)
        );

        Events_Push(
            CreateDodgeEvent(DODGE_RIGHT)
        );

        Events_Push(
            CreateDodgeEvent(DODGE_UP)
        );

        Events_Push(
            CreateDodgeEvent(DODGE_DOWN)
        );
    }

    Behavior_Update();

    Test_Assert(
        Float_Equals(
            Behavior_GetDodgeLeftProbability(),
            0.25f
        ),
        "Left probability is 25%"
    );

    Test_Assert(
        Float_Equals(
            Behavior_GetDodgeRightProbability(),
            0.25f
        ),
        "Right probability is 25%"
    );

    Test_Assert(
        Float_Equals(
            Behavior_GetDodgeUpProbability(),
            0.25f
        ),
        "Up probability is 25%"
    );

    Test_Assert(
        Float_Equals(
            Behavior_GetDodgeDownProbability(),
            0.25f
        ),
        "Down probability is 25%"
    );

    float total =
        Behavior_GetDodgeLeftProbability() +
        Behavior_GetDodgeRightProbability() +
        Behavior_GetDodgeUpProbability() +
        Behavior_GetDodgeDownProbability();

    Test_Assert(
        Float_Equals(total, 1.0f),
        "Dodge probabilities sum to 1"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 4
 * Movement statistics
 * ---------------------------------------------------------
 */

static void Test_MovementStatistics(void)
{
    Events_Init();
    Behavior_Init();

    for (int i = 0; i < 5; i++)
    {
        Events_Push(
            CreateMoveEvent(MOVE_LEFT)
        );
    }

    for (int i = 0; i < 3; i++)
    {
        Events_Push(
            CreateMoveEvent(MOVE_RIGHT)
        );
    }

    Events_Push(
        CreateMoveEvent(MOVE_UP)
    );

    Events_Push(
        CreateMoveEvent(MOVE_DOWN)
    );

    Behavior_Update();

    const PlayerBehavior *profile =
        Behavior_GetProfile();

    Test_Assert(
        profile->total_movements == 10,
        "Movement events are counted"
    );

    Test_Assert(
        profile->move_left_count == 5,
        "Left movement count is correct"
    );

    Test_Assert(
        profile->move_right_count == 3,
        "Right movement count is correct"
    );

    Test_Assert(
        profile->move_up_count == 1,
        "Up movement count is correct"
    );

    Test_Assert(
        profile->move_down_count == 1,
        "Down movement count is correct"
    );

    Test_Assert(
        Behavior_HasEnoughData() == true,
        "10 movements are enough data"
    );

    Test_Assert(
        Float_Equals(
            Behavior_GetMoveLeftProbability(),
            0.5f
        ),
        "Left movement probability is correct"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 5
 * Damage and healing statistics
 * ---------------------------------------------------------
 */

static void Test_DamageAndHealing(void)
{
    Events_Init();
    Behavior_Init();

    Events_Push(
        CreateDamageEvent(20.0f)
    );

    Events_Push(
        CreateDamageEvent(30.0f)
    );

    Events_Push(
        CreateDamageEvent(50.0f)
    );

    Events_Push(
        CreateHealEvent(40.0f)
    );

    Events_Push(
        CreateHealEvent(10.0f)
    );

    Behavior_Update();

    const PlayerBehavior *profile =
        Behavior_GetProfile();

    Test_Assert(
        profile->damage_events == 3,
        "Damage event count is correct"
    );

    Test_Assert(
        profile->total_damage_taken == 100,
        "Total damage taken is correct"
    );

    Test_Assert(
        Float_Equals(
            profile->average_damage_taken,
            100.0f / 3.0f
        ),
        "Average damage taken is correct"
    );

    Test_Assert(
        profile->total_healing == 50,
        "Total healing is correct"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 6
 * Invalid/zero-valued events
 * ---------------------------------------------------------
 */

static void Test_InvalidValues(void)
{
    Events_Init();
    Behavior_Init();

    Events_Push(
        CreateDamageEvent(-50.0f)
    );

    Events_Push(
        CreateHealEvent(-20.0f)
    );

    Behavior_Update();

    const PlayerBehavior *profile =
        Behavior_GetProfile();

    Test_Assert(
        profile->damage_events == 1,
        "Negative damage event is still counted"
    );

    Test_Assert(
        profile->total_damage_taken == 0,
        "Negative damage does not increase total damage"
    );

    Test_Assert(
        profile->total_healing == 0,
        "Negative healing does not increase total healing"
    );
}


/*
 * ---------------------------------------------------------
 * MAIN TEST RUNNER
 * ---------------------------------------------------------
 */

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" DungeonForge - Behavior Tests\n");
    printf("========================================\n\n");

    Test_EmptyProfile();

    Test_DodgeProbabilities();

    Test_AllDodgeDirections();

    Test_MovementStatistics();

    Test_DamageAndHealing();

    Test_InvalidValues();

    printf("\n========================================\n");
    printf(
        "Tests: %d | Passed: %d | Failed: %d\n",
        tests_run,
        tests_passed,
        tests_run - tests_passed
    );
    printf("========================================\n\n");

    return tests_run == tests_passed ? 0 : 1;
}