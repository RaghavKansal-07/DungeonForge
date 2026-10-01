#include <stdio.h>
#include <stdbool.h>

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


/*
 * ---------------------------------------------------------
 * EVENT CREATION HELPERS
 * ---------------------------------------------------------
 */

static GameEvent CreateEvent(
    EventType type,
    float value
)
{
    GameEvent event = {0};

    event.type = type;
    event.value = value;

    return event;
}


/*
 * ---------------------------------------------------------
 * TEST 1
 * Initialization
 * ---------------------------------------------------------
 */

static void Test_Initialization(void)
{
    Events_Init();

    Test_Assert(
        Events_GetCount() == 0,
        "Event queue initializes empty"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 2
 * Push
 * ---------------------------------------------------------
 */

static void Test_Push(void)
{
    Events_Init();

    GameEvent event =
        CreateEvent(EVENT_PLAYER_ATTACK, 25.0f);

    bool pushed = Events_Push(event);

    Test_Assert(
        pushed == true,
        "Event push succeeds"
    );

    Test_Assert(
        Events_GetCount() == 1,
        "Event count becomes one after push"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 3
 * Pop
 * ---------------------------------------------------------
 */

static void Test_Pop(void)
{
    Events_Init();

    GameEvent pushed_event =
        CreateEvent(EVENT_PLAYER_DAMAGE, 50.0f);

    Events_Push(pushed_event);

    GameEvent popped_event = {0};

    bool popped =
        Events_Pop(&popped_event);

    Test_Assert(
        popped == true,
        "Event pop succeeds"
    );

    Test_Assert(
        popped_event.type == EVENT_PLAYER_DAMAGE,
        "Popped event type is correct"
    );

    Test_Assert(
        popped_event.value == 50.0f,
        "Popped event value is correct"
    );

    Test_Assert(
        Events_GetCount() == 0,
        "Event count returns to zero after pop"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 4
 * FIFO ordering
 * ---------------------------------------------------------
 *
 * Events must be returned in the same order in which
 * they were inserted.
 */

static void Test_FIFOOrdering(void)
{
    Events_Init();

    Events_Push(
        CreateEvent(EVENT_PLAYER_ATTACK, 10.0f)
    );

    Events_Push(
        CreateEvent(EVENT_PLAYER_DODGE, 20.0f)
    );

    Events_Push(
        CreateEvent(EVENT_PLAYER_DAMAGE, 30.0f)
    );

    GameEvent event = {0};

    Events_Pop(&event);

    Test_Assert(
        event.type == EVENT_PLAYER_ATTACK &&
        event.value == 10.0f,
        "First event follows FIFO ordering"
    );

    Events_Pop(&event);

    Test_Assert(
        event.type == EVENT_PLAYER_DODGE &&
        event.value == 20.0f,
        "Second event follows FIFO ordering"
    );

    Events_Pop(&event);

    Test_Assert(
        event.type == EVENT_PLAYER_DAMAGE &&
        event.value == 30.0f,
        "Third event follows FIFO ordering"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 5
 * Empty queue
 * ---------------------------------------------------------
 */

static void Test_EmptyQueue(void)
{
    Events_Init();

    GameEvent event = {0};

    bool popped =
        Events_Pop(&event);

    Test_Assert(
        popped == false,
        "Pop fails when queue is empty"
    );

    Test_Assert(
        Events_GetCount() == 0,
        "Empty queue count remains zero"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 6
 * NULL output pointer
 * ---------------------------------------------------------
 */

static void Test_NullOutputPointer(void)
{
    Events_Init();

    Events_Push(
        CreateEvent(EVENT_PLAYER_ATTACK, 100.0f)
    );

    bool popped =
        Events_Pop(NULL);

    Test_Assert(
        popped == false,
        "Pop rejects NULL output pointer"
    );

    Test_Assert(
        Events_GetCount() == 1,
        "NULL pop does not remove event"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 7
 * Clear
 * ---------------------------------------------------------
 */

static void Test_Clear(void)
{
    Events_Init();

    Events_Push(
        CreateEvent(EVENT_PLAYER_ATTACK, 10.0f)
    );

    Events_Push(
        CreateEvent(EVENT_PLAYER_MOVE, 20.0f)
    );

    Events_Push(
        CreateEvent(EVENT_PLAYER_DODGE, 30.0f)
    );

    Test_Assert(
        Events_GetCount() == 3,
        "Events are present before clear"
    );

    Events_Clear();

    Test_Assert(
        Events_GetCount() == 0,
        "Clear removes all pending events"
    );

    GameEvent event = {0};

    Test_Assert(
        Events_Pop(&event) == false,
        "Pop fails after queue is cleared"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 8
 * Multiple events
 * ---------------------------------------------------------
 */

static void Test_MultipleEvents(void)
{
    Events_Init();

    for (int i = 0; i < 10; i++)
    {
        Events_Push(
            CreateEvent(
                EVENT_PLAYER_ATTACK,
                (float)i
            )
        );
    }

    Test_Assert(
        Events_GetCount() == 10,
        "Multiple events are counted correctly"
    );

    bool order_correct = true;

    for (int i = 0; i < 10; i++)
    {
        GameEvent event = {0};

        if (!Events_Pop(&event))
        {
            order_correct = false;
            break;
        }

        if (event.type != EVENT_PLAYER_ATTACK ||
            event.value != (float)i)
        {
            order_correct = false;
            break;
        }
    }

    Test_Assert(
        order_correct,
        "Multiple events preserve insertion order"
    );

    Test_Assert(
        Events_GetCount() == 0,
        "All multiple events are removed correctly"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 9
 * Queue capacity
 * ---------------------------------------------------------
 *
 * The production Event System defines a capacity of 256.
 *
 * The 256th event should succeed.
 * The 257th event should fail.
 */

static void Test_QueueCapacity(void)
{
    Events_Init();

    int successful_pushes = 0;

    for (int i = 0; i < 256; i++)
    {
        if (Events_Push(
                CreateEvent(
                    EVENT_PLAYER_MOVE,
                    (float)i
                )
            ))
        {
            successful_pushes++;
        }
    }

    Test_Assert(
        successful_pushes == 256,
        "Queue accepts 256 events"
    );

    Test_Assert(
        Events_GetCount() == 256,
        "Queue count reaches maximum capacity"
    );

    bool overflow_push =
        Events_Push(
            CreateEvent(
                EVENT_PLAYER_ATTACK,
                999.0f
            )
        );

    Test_Assert(
        overflow_push == false,
        "Queue rejects event when full"
    );

    Test_Assert(
        Events_GetCount() == 256,
        "Queue count does not exceed capacity"
    );
}


/*
 * ---------------------------------------------------------
 * TEST 10
 * Circular queue wrap-around
 * ---------------------------------------------------------
 *
 * This verifies that head/tail wrapping works correctly.
 */

static void Test_CircularQueueWrapAround(void)
{
    Events_Init();

    /*
     * Fill the queue.
     */
    for (int i = 0; i < 256; i++)
    {
        Events_Push(
            CreateEvent(
                EVENT_PLAYER_MOVE,
                (float)i
            )
        );
    }

    /*
     * Remove 128 events.
     *
     * This moves the head forward.
     */
    bool order_correct = true;

    for (int i = 0; i < 128; i++)
    {
        GameEvent event = {0};

        if (!Events_Pop(&event))
        {
            order_correct = false;
            break;
        }

        if (event.value != (float)i)
        {
            order_correct = false;
            break;
        }
    }

    Test_Assert(
        order_correct,
        "Initial events pop correctly before wrap-around"
    );

    /*
     * Add 128 new events.
     *
     * The tail must wrap around to the beginning
     * of the fixed-size array.
     */
    for (int i = 256; i < 384; i++)
    {
        if (!Events_Push(
                CreateEvent(
                    EVENT_PLAYER_MOVE,
                    (float)i
                )
            ))
        {
            order_correct = false;
            break;
        }
    }

    Test_Assert(
        Events_GetCount() == 256,
        "Queue remains full after circular wrap-around"
    );

    /*
     * The remaining old events should come first,
     * followed by the newly inserted events.
     */
    for (int expected = 128; expected < 384; expected++)
    {
        GameEvent event = {0};

        if (!Events_Pop(&event))
        {
            order_correct = false;
            break;
        }

        if (event.value != (float)expected)
        {
            order_correct = false;
            break;
        }
    }

    Test_Assert(
        order_correct,
        "Circular queue preserves FIFO order after wrap-around"
    );

    Test_Assert(
        Events_GetCount() == 0,
        "Queue becomes empty after wrapped events are consumed"
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
    printf(" DungeonForge - Event System Tests\n");
    printf("========================================\n\n");

    Test_Initialization();

    Test_Push();

    Test_Pop();

    Test_FIFOOrdering();

    Test_EmptyQueue();

    Test_NullOutputPointer();

    Test_Clear();

    Test_MultipleEvents();

    Test_QueueCapacity();

    Test_CircularQueueWrapAround();

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