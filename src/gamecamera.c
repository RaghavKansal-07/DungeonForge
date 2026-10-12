#include "gamecamera.h"

#include "player.h"
#include "tilemap.h"
#include "effects.h"

#include <math.h>
#include <stddef.h>

/* How quickly the camera catches up with the player. */
#define CAMERA_FOLLOW_SPEED 8.0f

/* The world position the camera is looking at. */
static Vector2 focus = {0.0f, 0.0f};

static float GameCamera_ClampAxis(
    float value,
    float world_size,
    float view_size)
{
    /* A world smaller than the screen is centered. */
    if (world_size <= view_size)
        return world_size * 0.5f;

    float half = view_size * 0.5f;

    if (value < half)
        return half;

    if (value > world_size - half)
        return world_size - half;

    return value;
}

static Vector2 GameCamera_Clamp(Vector2 position)
{
    position.x =
        GameCamera_ClampAxis(
            position.x,
            (float)(MAP_WIDTH * TILE_SIZE),
            (float)GetScreenWidth());

    position.y =
        GameCamera_ClampAxis(
            position.y,
            (float)(MAP_HEIGHT * TILE_SIZE),
            (float)GetScreenHeight());

    return position;
}

void GameCamera_Reset(void)
{
    Player *player =
        Player_Get();

    if (player == NULL)
        return;

    focus =
        GameCamera_Clamp(
            (Vector2){player->x, player->y});
}

void GameCamera_Update(float dt)
{
    Player *player =
        Player_Get();

    if (player == NULL)
        return;

    if (dt > 0.1f)
        dt = 0.1f;

    Vector2 target =
        GameCamera_Clamp(
            (Vector2){player->x, player->y});

    float blend =
        dt * CAMERA_FOLLOW_SPEED;

    if (blend > 1.0f)
        blend = 1.0f;

    focus.x += (target.x - focus.x) * blend;
    focus.y += (target.y - focus.y) * blend;
}

Camera2D GameCamera_Get(void)
{
    Vector2 shake =
        Effects_GetShake();

    Camera2D camera = {0};

    /*
     * Whole pixels only, so tile edges do not shimmer
     * while the camera moves.
     */
    camera.target =
        (Vector2){
            roundf(focus.x),
            roundf(focus.y)};

    camera.offset =
        (Vector2){
            roundf((float)GetScreenWidth() * 0.5f) + shake.x,
            roundf((float)GetScreenHeight() * 0.5f) + shake.y};

    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    return camera;
}

Vector2 GameCamera_WorldToScreen(
    float x,
    float y)
{
    Camera2D camera =
        GameCamera_Get();

    return (Vector2){
        x - camera.target.x + camera.offset.x,
        y - camera.target.y + camera.offset.y};
}

void GameCamera_GetVisibleTiles(
    int *min_x,
    int *min_y,
    int *max_x,
    int *max_y)
{
    Camera2D camera =
        GameCamera_Get();

    float left =
        camera.target.x - camera.offset.x;

    float top =
        camera.target.y - camera.offset.y;

    float right =
        left + (float)GetScreenWidth();

    float bottom =
        top + (float)GetScreenHeight();

    if (min_x != NULL)
        *min_x = (int)floorf(left / TILE_SIZE) - 1;

    if (min_y != NULL)
        *min_y = (int)floorf(top / TILE_SIZE) - 1;

    if (max_x != NULL)
        *max_x = (int)floorf(right / TILE_SIZE) + 1;

    if (max_y != NULL)
        *max_y = (int)floorf(bottom / TILE_SIZE) + 1;
}

Vector2 GameCamera_ScreenToWorld(
    Vector2 screen)
{
    Camera2D camera =
        GameCamera_Get();

    return (Vector2){
        screen.x - camera.offset.x + camera.target.x,
        screen.y - camera.offset.y + camera.target.y};
}