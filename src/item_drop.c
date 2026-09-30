#include "item_drop.h"

#include "player.h"
#include "raylib.h"
#include "audio.h"

#include <math.h>


static ItemDrop item_drops[ITEM_DROP_MAX];

static int item_drop_count = 0;


void ItemDrop_Init(void)
{
    item_drop_count = 0;

    for (int i = 0;
         i < ITEM_DROP_MAX;
         i++)
    {
        item_drops[i].active = false;
        item_drops[i].x = 0.0f;
        item_drops[i].y = 0.0f;
        item_drops[i].item_id = ITEM_ID_NONE;
        item_drops[i].quantity = 0;
    }
}


bool ItemDrop_Spawn(
    float x,
    float y,
    ItemId item_id,
    int quantity
)
{
    if (!Items_IsValid(item_id))
        return false;

    if (quantity <= 0)
        return false;

    if (item_drop_count >= ITEM_DROP_MAX)
        return false;

    for (int i = 0;
         i < ITEM_DROP_MAX;
         i++)
    {
        if (item_drops[i].active)
            continue;

        item_drops[i].active = true;
        item_drops[i].x = x;
        item_drops[i].y = y;
        item_drops[i].item_id = item_id;
        item_drops[i].quantity = quantity;

        item_drop_count++;

        return true;
    }

    return false;
}


void ItemDrop_Update(void)
{
    Player *player =
        Player_Get();

    if (player == NULL ||
        Player_IsDead())
    {
        return;
    }

    /*
     * Check every active drop against
     * the player's position.
     */
    for (int i = 0;
         i < ITEM_DROP_MAX;
         i++)
    {
        if (!item_drops[i].active)
            continue;

        float dx =
            player->x -
            item_drops[i].x;

        float dy =
            player->y -
            item_drops[i].y;

        float distance =
            sqrtf(
                dx * dx +
                dy * dy
            );

        /*
         * Automatically pick up an item
         * when the player gets close enough.
         */
        if (distance <= 28.0f)
        {
            if (Inventory_AddItem(
                    &player->inventory,
                    (ItemType)Items_GetDefinition(
                        item_drops[i].item_id
                    )->type,
                    item_drops[i].item_id,
                    item_drops[i].quantity))
            {
                Audio_PlayItemPickup();
                item_drops[i].active = false;
                item_drops[i].item_id =
                    ITEM_ID_NONE;
                item_drops[i].quantity = 0;

                item_drop_count--;
            }
        }
    }
}


void ItemDrop_Render(void)
{
    for (int i = 0;
         i < ITEM_DROP_MAX;
         i++)
    {
        if (!item_drops[i].active)
            continue;

        const ItemDefinition *definition =
            Items_GetDefinition(
                item_drops[i].item_id
            );

        if (definition == NULL)
            continue;

        /*
         * Different colors will eventually
         * represent different item categories.
         */
        Color item_color = GOLD;

        if (definition->type ==
            ITEM_HEALTH_POTION)
        {
            item_color = RED;
        }
        else if (definition->type ==
                 ITEM_WEAPON)
        {
            item_color = ORANGE;
        }
        else if (definition->type ==
                 ITEM_ARMOR)
        {
            item_color = SKYBLUE;
        }

        DrawCircle(
            (int)item_drops[i].x,
            (int)item_drops[i].y,
            8.0f,
            item_color
        );

        DrawText(
            definition->name,
            (int)item_drops[i].x + 12,
            (int)item_drops[i].y - 7,
            12,
            WHITE
        );
    }
}


int ItemDrop_GetCount(void)
{
    return item_drop_count;
}


ItemDrop *ItemDrop_Get(
    int index
)
{
    if (index < 0 ||
        index >= ITEM_DROP_MAX)
    {
        return NULL;
    }

    if (!item_drops[index].active)
        return NULL;

    return &item_drops[index];
}