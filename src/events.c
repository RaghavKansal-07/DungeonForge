#include "events.h"

#include <stddef.h>

/*
 * ---------------------------------------------------------
 * Event Queue Configuration
 * ---------------------------------------------------------
 *
 * A fixed-size queue is used deliberately.
 *
 * Gameplay events are small and short-lived, so we don't
 * need dynamic memory allocation here.
 */

#define EVENT_QUEUE_CAPACITY 256


/*
 * ---------------------------------------------------------
 * Event Queue
 * ---------------------------------------------------------
 */

static GameEvent event_queue[EVENT_QUEUE_CAPACITY];

static int event_head = 0;
static int event_tail = 0;
static int event_count = 0;


/*
 * ---------------------------------------------------------
 * Events_Init
 * ---------------------------------------------------------
 *
 * Reset the complete event queue.
 */

void Events_Init(void)
{
    event_head = 0;
    event_tail = 0;
    event_count = 0;
}


/*
 * ---------------------------------------------------------
 * Events_Push
 * ---------------------------------------------------------
 *
 * Add one event to the queue.
 *
 * Returns:
 *
 *     true  -> event successfully added
 *     false -> queue is full
 */

bool Events_Push(GameEvent event)
{
    if (event_count >= EVENT_QUEUE_CAPACITY)
        return false;

    event_queue[event_tail] = event;

    event_tail++;

    if (event_tail >= EVENT_QUEUE_CAPACITY)
        event_tail = 0;

    event_count++;

    return true;
}


/*
 * ---------------------------------------------------------
 * Events_Pop
 * ---------------------------------------------------------
 *
 * Remove the oldest event from the queue.
 *
 * Returns:
 *
 *     true  -> event successfully retrieved
 *     false -> queue is empty or output pointer invalid
 */

bool Events_Pop(GameEvent *event)
{
    if (event == NULL)
        return false;

    if (event_count <= 0)
        return false;

    *event = event_queue[event_head];

    event_head++;

    if (event_head >= EVENT_QUEUE_CAPACITY)
        event_head = 0;

    event_count--;

    return true;
}


/*
 * ---------------------------------------------------------
 * Events_GetCount
 * ---------------------------------------------------------
 */

int Events_GetCount(void)
{
    return event_count;
}


/*
 * ---------------------------------------------------------
 * Events_Clear
 * ---------------------------------------------------------
 *
 * Remove all pending events.
 */

void Events_Clear(void)
{
    event_head = 0;
    event_tail = 0;
    event_count = 0;
}