#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "attack_cone.h"

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

/*
 * Attack at the origin: range 55, half angle 55 degrees,
 * target radius 20 unless stated otherwise.
 */
static bool Hit(
    float direction_x,
    float direction_y,
    float target_x,
    float target_y,
    float target_radius)
{
    return AttackCone_Contains(
        0.0f, 0.0f,
        direction_x, direction_y,
        target_x, target_y,
        target_radius,
        55.0f,
        55.0f);
}

static void Test_Directions(void)
{
    Test_Assert(
        Hit(1, 0, 40, 0, 20),
        "A target straight ahead is hit");

    Test_Assert(
        !Hit(1, 0, -40, 0, 20),
        "A target directly behind is not hit");

    Test_Assert(
        !Hit(1, 0, 0, 40, 20),
        "A target at 90 degrees is not hit");

    Test_Assert(
        Hit(1, 0, 40 * cosf(0.6981f), 40 * sinf(0.6981f), 20),
        "A target 40 degrees off the aim is hit");

    Test_Assert(
        Hit(0, -1, 0, -40, 20),
        "Aiming up hits a target above");

    Test_Assert(
        !Hit(0, -1, 0, 40, 20),
        "Aiming up does not hit a target below");

    Test_Assert(
        Hit(-1, 0, -40, 0, 20),
        "Aiming left hits a target on the left");

    Test_Assert(
        Hit(1, 1, 30, 30, 20),
        "Aiming diagonally hits a target on the diagonal");

    Test_Assert(
        !Hit(1, 1, 30, -30, 20),
        "Aiming diagonally does not hit the opposite diagonal");
}

static void Test_Range(void)
{
    /* Reach is range + radius = 75 for a radius-20 target. */
    Test_Assert(
        Hit(1, 0, 74.9f, 0, 20),
        "A target just inside the reach is hit");

    Test_Assert(
        !Hit(1, 0, 75.1f, 0, 20),
        "A target just outside the reach is not hit");

    Test_Assert(
        Hit(1, 0, 82.0f, 0, 28),
        "A larger target is hit from farther away");

    Test_Assert(
        !Hit(1, 0, 200, 0, 20),
        "A distant target is not hit");
}

static void Test_TargetSize(void)
{
    float angle = 70.0f * 0.0174532925f;

    float x = 50.0f * cosf(angle);
    float y = 50.0f * sinf(angle);

    Test_Assert(
        !Hit(1, 0, x, y, 5),
        "A small target 70 degrees off is outside the cone");

    Test_Assert(
        Hit(1, 0, x, y, 30),
        "A large target 70 degrees off is hit because its edge is inside");
}

static void Test_EdgeCases(void)
{
    Test_Assert(
        Hit(1, 0, 0, 0, 20),
        "A target on top of the attacker is hit");

    Test_Assert(
        Hit(5, 0, 40, 0, 20),
        "The aim direction does not need to be normalized");

    Test_Assert(
        !Hit(0, 0, 40, 0, 20),
        "A zero aim direction hits nothing");

    Test_Assert(
        !Hit(NAN, 0, 40, 0, 20),
        "An invalid aim direction hits nothing");

    Test_Assert(
        AttackCone_Contains(
            100, 100, 1, 0, 140, 100, 20, 55, 55),
        "The attack works from a position other than the origin");
}

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" DungeonForge - Attack Cone Tests\n");
    printf("========================================\n\n");

    Test_Directions();

    Test_Range();

    Test_TargetSize();

    Test_EdgeCases();

    printf("\n========================================\n");
    printf(
        "Tests: %d | Passed: %d | Failed: %d\n",
        tests_run,
        tests_passed,
        tests_run - tests_passed);
    printf("========================================\n\n");

    return tests_run == tests_passed ? 0 : 1;
}