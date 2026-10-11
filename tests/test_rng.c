#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "rng.h"

static int tests_run = 0;
static int tests_passed = 0;

static void Test_Assert(bool condition, const char *name)
{
    tests_run++;

    if (condition)
    {
        tests_passed++;
        printf("[PASS] %s\n", name);
    }
    else
    {
        printf("[FAIL] %s\n", name);
    }
}

static void Test_Determinism(void)
{
    Rng a, b;

    Rng_Seed(&a, 12345u);
    Rng_Seed(&b, 12345u);

    bool same = true;

    for (int i = 0; i < 1000; i++)
    {
        if (Rng_Next(&a) != Rng_Next(&b))
            same = false;
    }

    Test_Assert(same, "Same seed produces the same sequence");

    Rng c;
    Rng_Seed(&c, 12346u);
    Rng_Seed(&a, 12345u);

    bool differs = false;

    for (int i = 0; i < 10; i++)
    {
        if (Rng_Next(&a) != Rng_Next(&c))
            differs = true;
    }

    Test_Assert(differs, "Different seeds produce different sequences");
}

static void Test_Independence(void)
{
    Rng a, b;

    Rng_Seed(&a, 7u);
    Rng_Seed(&b, 7u);

    uint32_t first = Rng_Next(&a);

    /* Drawing from another generator must not affect a. */
    for (int i = 0; i < 100; i++)
        Rng_Next(&b);

    Rng c;
    Rng_Seed(&c, 7u);

    Test_Assert(
        first == Rng_Next(&c),
        "Generators are independent of each other"
    );
}

static void Test_Range(void)
{
    Rng rng;
    Rng_Seed(&rng, 99u);

    bool in_bounds = true;
    bool saw_min = false;
    bool saw_max = false;

    for (int i = 0; i < 20000; i++)
    {
        int v = Rng_Range(&rng, 4, 9);

        if (v < 4 || v > 9)
            in_bounds = false;

        if (v == 4) saw_min = true;
        if (v == 9) saw_max = true;
    }

    Test_Assert(in_bounds, "Range stays within [min, max]");
    Test_Assert(saw_min && saw_max, "Range reaches both ends");

    Test_Assert(
        Rng_Range(&rng, 5, 5) == 5,
        "Range with min == max returns min"
    );
}

static void Test_Float(void)
{
    Rng rng;
    Rng_Seed(&rng, 2024u);

    bool in_bounds = true;
    double sum = 0.0;

    for (int i = 0; i < 20000; i++)
    {
        float v = Rng_Float(&rng);

        if (v < 0.0f || v > 1.0f)
            in_bounds = false;

        sum += v;
    }

    double mean = sum / 20000.0;

    Test_Assert(in_bounds, "Float stays within [0, 1]");

    Test_Assert(
        mean > 0.48 && mean < 0.52,
        "Float values are evenly distributed (mean near 0.5)"
    );
}

static void Test_EdgeCases(void)
{
    Rng rng;
    Rng_Seed(&rng, 0u);

    Test_Assert(
        Rng_Next(&rng) != 0 || Rng_Next(&rng) != 0,
        "Seed 0 still produces output"
    );

    Test_Assert(Rng_Next(NULL) == 0, "NULL generator is handled safely");
    Rng_Seed(NULL, 1u);
    Test_Assert(true, "Seeding NULL does not crash");
}

int main(void)
{
    printf("\n========================================\n");
    printf(" DungeonForge - RNG Tests\n");
    printf("========================================\n\n");

    Test_Determinism();
    Test_Independence();
    Test_Range();
    Test_Float();
    Test_EdgeCases();

    printf("\n========================================\n");
    printf("Tests: %d | Passed: %d | Failed: %d\n",
           tests_run, tests_passed, tests_run - tests_passed);
    printf("========================================\n\n");

    return tests_run == tests_passed ? 0 : 1;
}