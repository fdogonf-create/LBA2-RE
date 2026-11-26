#ifndef ACTIONHINT_H
#define ACTIONHINT_H

#include <SYSTEM/ADELINE_TYPES.H>

/* A small "A" button drawn in the top-right corner while the action button
 * would do something: a character or object near Twinsen whose script is asking
 * whether action was pressed, a bonus zone not yet taken, a message zone. The
 * game keeps no list of what can be talked to, so this is the script's own
 * question, noticed as it is asked; it can show for something that answers
 * only at some point of the story.
 *
 * Community addition, on in a build with LBA2_MODERN_PAD_DEFAULT (the Switch
 * build) and off elsewhere. cfg GamepadActionHint, cvar pad_action_hint. */
#ifndef LBA2_ACTION_HINT_DEFAULT
#if defined(LBA2_MODERN_PAD_DEFAULT) && LBA2_MODERN_PAD_DEFAULT
#define LBA2_ACTION_HINT_DEFAULT 1
#else
#define LBA2_ACTION_HINT_DEFAULT 0
#endif
#endif

/* How close (world units) the object asking must be to Twinsen, and how far
 * off his facing (4096 a turn) it may stand. */
#define ACTION_HINT_REACH 2000
#define ACTION_HINT_CONE 768
/* How long a hint stays up after the last frame that asked for it, so it does
 * not flicker on scripts that ask every other frame, yet goes away as soon as a
 * dialogue or a menu stops the scripts. */
#define ACTION_HINT_HOLD_MS 150

extern S32 GamepadActionHint;

/* Whether an object at (dx, dz) from Twinsen, who faces `beta`, is close
 * enough and in front enough to count. Pure, for the host test. */
extern S32 ActionHint_InReach(S32 dx, S32 dz, S32 beta);

/* Something can be done now. `nowMs` is the platform clock. */
extern void ActionHint_Note(U32 nowMs);

/* Whether the hint is up at `nowMs`. */
extern S32 ActionHint_Visible(U32 nowMs);

/* Draw it onto the presented ARGB frame (the PrePresent signature). */
extern void ActionHint_Render(U32 *frameBuffer, U32 pitch, U32 width, U32 height, const U8 *palette_rgb);

#endif
