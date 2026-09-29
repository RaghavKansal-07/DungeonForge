#ifndef EVENTS_H
#define EVENTS_H

#include <stdbool.h>

/*
 * ---------------------------------------------------------
 * Event Types
 * ---------------------------------------------------------
 *
 * These represent meaningful gameplay actions that
 * other systems may want to observe.
 */

typedef enum
{
    EVENT_NONE,

    EVENT_PLAYER_ATTACK,
    EVENT_PLAYER_DODGE,
    EVENT_PLAYER_MOVE,
    EVENT_PLAYER_DAMAGE,
    EVENT_PLAYER_HEAL,

    EVENT_ENEMY_DAMAGED

} EventType;


/*
 * ---------------------------------------------------------
 * Dodge Direction
 * ---------------------------------------------------------
 */

typedef enum
{
    DODGE_NONE,

    DODGE_LEFT,
    DODGE_RIGHT,
    DODGE_UP,
    DODGE_DOWN

} DodgeDirection;


/*
 * ---------------------------------------------------------
 * Movement Direction
 * ---------------------------------------------------------
 */

typedef enum
{
    MOVE_NONE,

    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN

} MoveDirection;


/*
 * ---------------------------------------------------------
 * Game Event
 * ---------------------------------------------------------
 *
 * A single event represents one meaningful gameplay
 * occurrence.
 *
 * The structure is intentionally generic so the event
 * system can later be extended without redesigning the
 * entire architecture.
 */

typedef struct
{
    EventType type;

    /*
     * Direction associated with the event.
     *
     * Used by movement and dodge events.
     */
    DodgeDirection dodge_direction;

    MoveDirection move_direction;

    /*
     * Generic numeric information.
     *
     * Examples:
     *
     *     attack damage
     *     received damage
     *     healing amount
     */
    float value;

    /*
     * Position at which the event occurred.
     */
    float x;
    float y;

} GameEvent;


/*
 * ---------------------------------------------------------
 * Event System
 * ---------------------------------------------------------
 */

void Events_Init(void);

bool Events_Push(
    GameEvent event
);

bool Events_Pop(
    GameEvent *event
);

int Events_GetCount(void);

void Events_Clear(void);

#endif