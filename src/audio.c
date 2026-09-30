#include "audio.h"

/*
 * DungeonForge Audio Manager
 *
 * All game sound effects are loaded and managed here.
 * Gameplay systems do not need to know how the sounds
 * are stored or loaded.
 */

/* -------------------------------------------------------------------------- */
/* Audio assets                                                               */
/* -------------------------------------------------------------------------- */

static Sound player_attack_sound;
static Sound player_damage_sound;

static Sound enemy_hit_sound;
static Sound enemy_death_sound;

static Sound item_pickup_sound;

static Sound boss_attack_sound;
static Sound boss_phase_sound;

/* -------------------------------------------------------------------------- */
/* Audio manager state                                                        */
/* -------------------------------------------------------------------------- */

static bool audio_initialized = false;
static bool audio_enabled = true;

static float master_volume = 0.75f;

/* -------------------------------------------------------------------------- */
/* Internal helpers                                                           */
/* -------------------------------------------------------------------------- */

static float Audio_ClampVolume(float volume)
{
    if (volume < 0.0f)
    {
        return 0.0f;
    }

    if (volume > 1.0f)
    {
        return 1.0f;
    }

    return volume;
}

static void Audio_ApplyVolume(void)
{
    SetSoundVolume(player_attack_sound, master_volume);
    SetSoundVolume(player_damage_sound, master_volume);

    SetSoundVolume(enemy_hit_sound, master_volume);
    SetSoundVolume(enemy_death_sound, master_volume);

    SetSoundVolume(item_pickup_sound, master_volume);

    SetSoundVolume(boss_attack_sound, master_volume);
    SetSoundVolume(boss_phase_sound, master_volume);
}

/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

bool Audio_Init(void)
{
    if (audio_initialized)
    {
        return true;
    }

    /*
     * InitAudioDevice() returns void in the raylib version
     * used by DungeonForge.
     */
    InitAudioDevice();

    player_attack_sound =
        LoadSound("assets/audio/player_attack.ogg");

    player_damage_sound =
        LoadSound("assets/audio/player_damage.ogg");

    if (!IsSoundValid(player_damage_sound))
    {
        TraceLog(
            LOG_ERROR,
            "Failed to load player_damage.ogg");
    }
    else
    {
        TraceLog(
            LOG_INFO,
            "Loaded player_damage.ogg successfully");
    }

    enemy_hit_sound =
        LoadSound("assets/audio/enemy_hit.ogg");

    enemy_death_sound =
        LoadSound("assets/audio/enemy_death.ogg");

    item_pickup_sound =
        LoadSound("assets/audio/item_pickup.ogg");

    boss_attack_sound =
        LoadSound("assets/audio/boss_attack.ogg");

    boss_phase_sound =
        LoadSound("assets/audio/boss_phase.ogg");

    audio_initialized = true;

    Audio_ApplyVolume();

    return true;
}

/* -------------------------------------------------------------------------- */
/* Player sounds                                                              */
/* -------------------------------------------------------------------------- */

void Audio_PlayPlayerAttack(void)
{
    if (!audio_initialized || !audio_enabled)
    {
        return;
    }

    PlaySound(player_attack_sound);
}

void Audio_PlayPlayerDamage(void)
{
    if (!audio_initialized || !audio_enabled)
    {
        return;
    }

    PlaySound(player_damage_sound);
}

/* -------------------------------------------------------------------------- */
/* Enemy sounds                                                               */
/* -------------------------------------------------------------------------- */

void Audio_PlayEnemyHit(void)
{
    if (!audio_initialized || !audio_enabled)
    {
        return;
    }

    PlaySound(enemy_hit_sound);
}

void Audio_PlayEnemyDeath(void)
{
    if (!audio_initialized || !audio_enabled)
    {
        return;
    }

    PlaySound(enemy_death_sound);
}

/* -------------------------------------------------------------------------- */
/* Item sounds                                                                */
/* -------------------------------------------------------------------------- */

void Audio_PlayItemPickup(void)
{
    if (!audio_initialized || !audio_enabled)
    {
        return;
    }

    PlaySound(item_pickup_sound);
}

/* -------------------------------------------------------------------------- */
/* Boss sounds                                                                */
/* -------------------------------------------------------------------------- */

void Audio_PlayBossAttack(void)
{
    if (!audio_initialized || !audio_enabled)
    {
        return;
    }

    PlaySound(boss_attack_sound);
}

void Audio_PlayBossPhase(void)
{
    if (!audio_initialized || !audio_enabled)
    {
        return;
    }

    PlaySound(boss_phase_sound);
}

/* -------------------------------------------------------------------------- */
/* Volume control                                                             */
/* -------------------------------------------------------------------------- */

void Audio_SetMasterVolume(float volume)
{
    master_volume = Audio_ClampVolume(volume);

    if (audio_initialized)
    {
        Audio_ApplyVolume();
    }
}

float Audio_GetMasterVolume(void)
{
    return master_volume;
}

/* -------------------------------------------------------------------------- */
/* Enable / disable                                                           */
/* -------------------------------------------------------------------------- */

void Audio_SetEnabled(bool enabled)
{
    audio_enabled = enabled;

    if (!audio_enabled)
    {
        Audio_StopAll();
    }
}

bool Audio_IsEnabled(void)
{
    return audio_enabled;
}

/* -------------------------------------------------------------------------- */
/* Stop all                                                                   */
/* -------------------------------------------------------------------------- */

void Audio_StopAll(void)
{
    if (!audio_initialized)
    {
        return;
    }

    StopSound(player_attack_sound);
    StopSound(player_damage_sound);

    StopSound(enemy_hit_sound);
    StopSound(enemy_death_sound);

    StopSound(item_pickup_sound);

    StopSound(boss_attack_sound);
    StopSound(boss_phase_sound);
}

/* -------------------------------------------------------------------------- */
/* Shutdown                                                                   */
/* -------------------------------------------------------------------------- */

void Audio_Shutdown(void)
{
    if (!audio_initialized)
    {
        return;
    }

    Audio_StopAll();

    UnloadSound(player_attack_sound);
    UnloadSound(player_damage_sound);

    UnloadSound(enemy_hit_sound);
    UnloadSound(enemy_death_sound);

    UnloadSound(item_pickup_sound);

    UnloadSound(boss_attack_sound);
    UnloadSound(boss_phase_sound);

    CloseAudioDevice();

    audio_initialized = false;
}