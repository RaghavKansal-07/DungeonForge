#ifndef AUDIO_H
#define AUDIO_H

/*
 * DungeonForge Audio Manager
 *
 * Handles loading, playback, volume control,
 * and cleanup of all game sound effects.
 */

#include <stdbool.h>
#include "raylib.h"

/*
 * Initialize the audio system and load
 * all DungeonForge sound effects.
 *
 * Returns true when initialization succeeds.
 */
bool Audio_Init(void);

/*
 * Play a specific game sound.
 */
void Audio_PlayPlayerAttack(void);
void Audio_PlayPlayerDamage(void);

void Audio_PlayEnemyHit(void);
void Audio_PlayEnemyDeath(void);

void Audio_PlayItemPickup(void);

void Audio_PlayBossAttack(void);
void Audio_PlayBossPhase(void);

/*
 * Set the master volume for game sound effects.
 *
 * Expected range:
 *     0.0f = silent
 *     1.0f = maximum
 */
void Audio_SetMasterVolume(float volume);

/*
 * Get the current master volume.
 */
float Audio_GetMasterVolume(void);

/*
 * Enable or disable game sound effects.
 */
void Audio_SetEnabled(bool enabled);

/*
 * Check whether game sound effects are enabled.
 */
bool Audio_IsEnabled(void);

/*
 * Stop all currently playing game sounds.
 */
void Audio_StopAll(void);

/*
 * Unload all audio resources and shut down
 * the audio manager.
 */
void Audio_Shutdown(void);

#endif