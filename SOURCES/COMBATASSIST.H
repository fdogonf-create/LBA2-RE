#ifndef COMBATASSIST_H
#define COMBATASSIST_H

#include <SYSTEM/ADELINE_TYPES.H>

/* Improved combat: a community addition for quick enemies, on in a build with
 * LBA2_MODERN_PAD_DEFAULT (the Switch build) and off elsewhere, where the game
 * fights as it always has. cfg CombatAssist, cvar combat_assist, and a row in
 * Advanced options. In the original a hit that knocks Twinsen back plays its
 * reaction to the end with the controls ignored, and cancels a blow he had
 * started, so an enemy that strikes again before the reaction ends never lets
 * him answer. With it on:
 *
 *  - in Aggressive, pressing attack during that reaction cuts it short and
 *    strikes at once;
 *  - for COMBAT_ASSIST_GRACE_MS after a reaction starts, further hits still
 *    take his life points but no longer knock him back or cancel his blow.
 *
 * The damage the enemies do is unchanged. Defined in OBJECT.CPP. */
#ifndef LBA2_COMBAT_ASSIST_DEFAULT
#if defined(LBA2_MODERN_PAD_DEFAULT) && LBA2_MODERN_PAD_DEFAULT
#define LBA2_COMBAT_ASSIST_DEFAULT 1
#else
#define LBA2_COMBAT_ASSIST_DEFAULT 0
#endif
#endif

/* Game time (TimerRefHR, ms) after a knock-back during which hits do not knock
 * him back again. */
#define COMBAT_ASSIST_GRACE_MS 1000

extern S32 CombatAssist;

#endif
