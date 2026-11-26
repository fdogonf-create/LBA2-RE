/*--------------------------------------------------------------------------*/
#ifndef ANALOGMOVE_H
#define ANALOGMOVE_H

#include <SYSTEM/ADELINE_TYPES.H>

/* Analog movement: Twinsen walks the way the left stick points, as seen on
 * screen, instead of the original tank controls (forward on a button, the stick
 * turning him). Off by default; GamepadAnalogMove in lba2.cfg, `pad_analog_move`
 * in the console. Only the gamepad's left stick drives it: keyboard, D-pad and
 * triggers keep the original controls, and a stick at rest leaves the frame to
 * them. Ladders and vehicles keep the original controls too.
 *
 * Angles are the engine's: 4096 to a turn, 0 = +Z, 1024 = +X, and a larger
 * angle is a turn to Twinsen's left. */

#ifndef LBA2_ANALOG_MOVE_DEFAULT
#define LBA2_ANALOG_MOVE_DEFAULT 0
#endif

/* Turn speed while steering, in angle units per second (8192 = two turns). */
#define ANALOG_MOVE_TURN_RATE 8192
/* Past this angle between Twinsen and the stick he turns on the spot before
 * walking, so a reversal does not walk him round a wide circle. */
#define ANALOG_MOVE_WALK_MAX_DIFF 1024

#ifndef LBA2_MODERN_PAD_DEFAULT
#define LBA2_MODERN_PAD_DEFAULT 0
#endif

extern S32 GamepadAnalogMove;     /* 1 = left stick moves Twinsen the way it points */
extern S32 GamepadDpadZoom;       /* 1 = D-pad up / down zoom the Auto camera in / out */
extern S32 GamepadBehaviourCycle; /* 1 = a tap of ZL steps to the next behaviour */
extern S32 GamepadCameraCurve;    /* 1 = right stick answers small tilts gently (squared ramp) */

/* Zoom per frame while the D-pad is held, in camera distance units: the full
 * range (FOLLOW_CAM_DIST_MIN..MAX) in about two seconds. */
#define ANALOG_MOVE_DPAD_ZOOM_STEP 110

/* True on the frame ZL goes down with GamepadBehaviourCycle on; the caller
 * turns it into the next behaviour. Held, it does not repeat. */
S32 AnalogMove_BehaviourCyclePressed(void);

extern S32 GamepadQuickMenu; /* 1 = R opens the quick item menu */
/* True on the frame R goes down with GamepadQuickMenu on. */
S32 AnalogMove_QuickMenuPressed(void);

/* The D-pad's zoom for this frame: positive in (up), negative out (down), 0
 * with the setting off or neither held. */
S32 AnalogMove_DpadZoom(void);

/* A right-stick step after the response curve (pure): `step` scaled by how far
 * past the deadzone the stick is (0..1024), and by that again when `curved`, so a
 * small tilt turns the camera slowly and only a full one reaches `step`. */
S32 AnalogMove_CurveStep(S32 step, S32 travel1024, S32 curved);

/* The hero's manual move, before it reads Input: with the stick held, turns
 * `*beta` toward the stick's direction (unless `canTurn` is 0, as mid-jump) and
 * replaces Input's movement bits with a step forward once he faces near enough.
 * `exterior` reads the stick against the 3D camera at yaw `betaCam`, otherwise
 * against the isometric view; `cameraOrbiting` (the player turning the camera)
 * takes the camera's yaw again. `onLadder` (climbing) reads the stick as up =
 * climb, down = come down. `nearLadder` (in a ladder or window zone, not yet on
 * it) moves him as usual, so walking into the wall starts the climb, but a stick
 * held against his facing steps him backwards instead of turning him round, the
 * way the original comes down a ladder from the top. */
void AnalogMove_Apply(S32 *beta, S32 canTurn, S32 exterior, S32 onLadder, S32 nearLadder, S32 betaCam,
                      S32 cameraOrbiting);

/* The buggy's manual drive, before it reads Input: with the stick move on, ZR
 * holds the accelerator (I_UP) and ZL the brake and reverse (I_DOWN); the left
 * stick's left and right still steer. Outside the buggy ZR and ZL keep their
 * usual jobs. */
void AnalogMove_ApplyBuggy(void);

/* Pure helpers (host-tested). Stick values are SDL axis units, Y+ = down. */

/* Signed shortest turn from `from` to `to`, in -2048..2047. */
S32 AnalogMove_AngleDiff(S32 from, S32 to);

/* `beta` turned toward `target` by at most `maxStep`, wrapped to 0..4095. */
S32 AnalogMove_StepToward(S32 beta, S32 target, S32 maxStep);

/* The stick's direction relative to "up" on the stick: 0 = up, 1024 = left,
 * 2048 = down, 3072 = right. */
S32 AnalogMove_StickAngle(S32 x, S32 y);

/* World facing for a stick held this way in an exterior scene, where the 3D
 * camera at yaw `betaCam` looks along 2048 - betaCam. */
S32 AnalogMove_TargetExterior(S32 x, S32 y, S32 betaCam);

/* World facing for a stick held this way in an isometric interior: screen up is
 * world -X-Z, screen right +X-Z, with the projection's 2:1 vertical squash
 * undone so a diagonal on the stick is a diagonal on screen. */
S32 AnalogMove_TargetIso(S32 x, S32 y);

#endif /* ANALOGMOVE_H */
