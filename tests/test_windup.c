#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stddef.h>

#include "windup.h"

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

static bool Near(float a, float b)
{
    return fabsf(a - b) < 0.001f;
}

static void Test_Inactive(void)
{
    Windup w;

    Windup_Reset(&w);

    Test_Assert(
        !Windup_IsActive(&w),
        "A fresh wind-up is inactive");

    Test_Assert(
        Near(Windup_GetProgress(&w), 0.0f),
        "An inactive wind-up has no progress");

    Test_Assert(
        !Windup_Update(&w, 1.0f),
        "Updating an inactive wind-up never completes");
}

static void Test_Lifecycle(void)
{
    Windup w;

    Windup_Reset(&w);
    Windup_Start(&w, 0.5f);

    Test_Assert(
        Windup_IsActive(&w) &&
            Near(Windup_GetProgress(&w), 0.0f),
        "A started wind-up is active and at 0 progress");

    bool early = Windup_Update(&w, 0.25f);

    Test_Assert(
        !early &&
            Windup_IsActive(&w) &&
            Near(Windup_GetProgress(&w), 0.5f),
        "Halfway through it is active at 50 percent and has not landed");

    bool done = Windup_Update(&w, 0.25f);

    Test_Assert(
        done &&
            !Windup_IsActive(&w),
        "It completes when the time is up and becomes inactive");

    Test_Assert(
        !Windup_Update(&w, 0.25f),
        "It completes only once");
}

static void Test_Cancel(void)
{
    Windup w;

    Windup_Reset(&w);
    Windup_Start(&w, 0.5f);

    Windup_Update(&w, 0.2f);
    Windup_Cancel(&w);

    Test_Assert(
        !Windup_IsActive(&w),
        "A cancelled wind-up is inactive");

    Test_Assert(
        !Windup_Update(&w, 1.0f),
        "A cancelled wind-up never lands");
}

static void Test_EdgeCases(void)
{
    Windup w;

    Windup_Reset(&w);
    Windup_Start(&w, 0.3f);

    Test_Assert(
        Windup_Update(&w, 5.0f),
        "A very long frame still completes the wind-up");

    Windup_Start(&w, 0.3f);

    Windup_Update(&w, -1.0f);

    Test_Assert(
        Near(Windup_GetProgress(&w), 0.0f),
        "A negative time step does not advance the wind-up");

    Windup_Start(&w, 0.0f);

    Test_Assert(
        Windup_IsActive(&w) &&
            Windup_Update(&w, 0.016f),
        "A zero duration completes on the next update");

    Windup_Start(&w, 0.4f);
    Windup_Update(&w, 0.4f);
    Windup_Start(&w, 0.4f);

    Test_Assert(
        Windup_IsActive(&w) &&
            Near(Windup_GetProgress(&w), 0.0f),
        "A wind-up can be started again after it completes");
}

static void Test_FrameSteps(void)
{
    Windup w;

    Windup_Reset(&w);
    Windup_Start(&w, 0.45f);

    int steps = 0;
    bool landed = false;
    bool monotonic = true;
    float last = -1.0f;

    while (!landed && steps < 1000)
    {
        steps++;

        landed = Windup_Update(&w, 1.0f / 60.0f);

        float progress = Windup_GetProgress(&w);

        if (!landed && progress < last)
            monotonic = false;

        last = progress;
    }

    Test_Assert(
        landed && steps >= 26 && steps <= 28,
        "A 0.45 second wind-up lands after about 27 frames at 60 FPS");

    Test_Assert(
        monotonic,
        "Progress never goes backwards while the wind-up runs");
}

static void Test_NullSafety(void)
{
    Windup_Reset(NULL);
    Windup_Start(NULL, 1.0f);
    Windup_Cancel(NULL);

    Test_Assert(
        !Windup_IsActive(NULL) &&
            !Windup_Update(NULL, 1.0f) &&
            Near(Windup_GetProgress(NULL), 0.0f),
        "All wind-up functions are safe with a NULL pointer");
}

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" DungeonForge - Wind-up Tests\n");
    printf("========================================\n\n");

    Test_Inactive();

    Test_Lifecycle();

    Test_Cancel();

    Test_EdgeCases();

    Test_FrameSteps();

    Test_NullSafety();

    printf("\n========================================\n");
    printf(
        "Tests: %d | Passed: %d | Failed: %d\n",
        tests_run,
        tests_passed,
        tests_run - tests_passed);
    printf("========================================\n\n");

    return tests_run == tests_passed ? 0 : 1;
}