#ifndef BALANCE_H
#define BALANCE_H

/*
 * ---------------------------------------------------------
 * GAME BALANCE
 * ---------------------------------------------------------
 *
 * Every number that decides how hard the game is lives here.
 * To make the game easier or harder, change this file only.
 *
 * tests/test_balance.c checks that these numbers keep a
 * sensible relationship to each other (reaction time, hits
 * to kill, cost of a one-on-one fight), so a careless
 * change is caught by `make test`.
 */

/* ---------------------------------------------------------
 * Player
 * --------------------------------------------------------- */

#define BAL_PLAYER_MAX_HEALTH 100

#define BAL_PLAYER_ATTACK_DAMAGE 20
#define BAL_PLAYER_ATTACK_COOLDOWN 0.50f

/*
 * While swinging, the player moves at this fraction of full speed
 * for BAL_PLAYER_SWING_TIME seconds, so attacking has a cost.
 */
#define BAL_PLAYER_SWING_TIME 0.18f
#define BAL_PLAYER_SWING_MOVE_SCALE 0.40f

/* Seconds of invulnerability after taking a hit. */
#define BAL_PLAYER_DAMAGE_IMMUNITY 0.45f

#define BAL_PLAYER_POTION_HEAL 25
#define BAL_PLAYER_POTION_COOLDOWN 4.0f

/* The most of one item the player can carry. */
#define BAL_INVENTORY_MAX_STACK 5

/* ---------------------------------------------------------
 * Normal enemies (shared)
 * --------------------------------------------------------- */

/* Distance from the enemy's center that its swing reaches. */
#define BAL_ENEMY_ATTACK_RANGE 72.0f

/* Extra reach the swing gets. The warning ring includes it. */
#define BAL_ENEMY_HIT_TOLERANCE 8.0f

/*
 * A hit only interrupts a swing during this first fraction of
 * the wind-up. After that the swing is committed.
 */
#define BAL_WINDUP_INTERRUPT_LIMIT 0.4f

#define BAL_ENEMY_POTION_DROP_CHANCE 0.40f

/*
 * Mercy rule: extra drop chance when the player needs healing.
 * The bonuses add to the base chance.
 */
#define BAL_POTION_DROP_BONUS_NO_POTIONS 0.25f
#define BAL_POTION_DROP_BONUS_LOW_HEALTH 0.15f

/* "Low health" for the mercy rule, as a percentage of max. */
#define BAL_POTION_LOW_HEALTH_PERCENT 40
#define BAL_BOSS_POTION_DROPS 2

/* ---------------------------------------------------------
 * Hunter: balanced
 * --------------------------------------------------------- */

#define BAL_HUNTER_HEALTH 100
#define BAL_HUNTER_SPEED 170.0f
#define BAL_HUNTER_DAMAGE 16
#define BAL_HUNTER_WINDUP 0.45f
#define BAL_HUNTER_COOLDOWN 0.30f

/* ---------------------------------------------------------
 * Guardian: slow tank
 * --------------------------------------------------------- */

#define BAL_GUARDIAN_HEALTH 120
#define BAL_GUARDIAN_SPEED 130.0f
#define BAL_GUARDIAN_DAMAGE 20
#define BAL_GUARDIAN_WINDUP 0.60f
#define BAL_GUARDIAN_COOLDOWN 0.30f

/* ---------------------------------------------------------
 * Assassin: fast glass cannon
 * --------------------------------------------------------- */

#define BAL_ASSASSIN_HEALTH 70
#define BAL_ASSASSIN_SPEED 195.0f
#define BAL_ASSASSIN_DAMAGE 18
#define BAL_ASSASSIN_WINDUP 0.35f
#define BAL_ASSASSIN_COOLDOWN 0.20f

/* ---------------------------------------------------------
 * Boss
 * --------------------------------------------------------- */

#define BAL_BOSS_HEALTH 600

#define BAL_BOSS_ATTACK_DAMAGE 24
#define BAL_BOSS_SPECIAL_DAMAGE 34

#define BAL_BOSS_ATTACK_RANGE 82.0f

/* Closer than this and the boss steps back to make room. */
#define BAL_BOSS_CONTACT_DISTANCE 64.0f

#define BAL_BOSS_SPECIAL_RANGE 130.0f
#define BAL_BOSS_SPECIAL_COOLDOWN 4.0f

#define BAL_BOSS_MELEE_WINDUP 0.50f
#define BAL_BOSS_SPECIAL_WINDUP 0.85f

/* ---------------------------------------------------------
 * Gear
 * --------------------------------------------------------- */

/* Iron Sword: extra damage on every player hit. */
#define BAL_SWORD_DAMAGE_BONUS 6

/* Iron Armor: percent of incoming damage that is blocked. */
#define BAL_ARMOR_REDUCTION_PERCENT 25

/* Chance that a kill drops the item (only if not already owned). */
#define BAL_HUNTER_SWORD_DROP_CHANCE 0.15f
#define BAL_ASSASSIN_SWORD_DROP_CHANCE 0.15f
#define BAL_GUARDIAN_ARMOR_DROP_CHANCE 0.30f

/* ---------------------------------------------------------
 * Boss reaction to a close-range fighter ("keep distance")
 * --------------------------------------------------------- */

/*
 * Against a brawler the boss backs away when the player is
 * closer than this. Must be between the normal contact distance
 * and the boss attack range.
 */
#define BAL_BOSS_KEEP_DISTANCE 76.0f

/* The boss special recharges this much faster (multiplier). */
#define BAL_BOSS_KEEP_SPECIAL_SCALE 0.60f

#endif