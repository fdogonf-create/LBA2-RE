/*--------------------------------------------------------------------------*/
/* Analog movement glue: turns the left stick into a facing and a step forward
 * for the hero's manual move. The angle math is in ANALOGMOVE_MATH.CPP. */
#include "ANALOGMOVE.H"
#include "INPUT.H"
#include "JOYSTICK.H"
#include "RECORD.H"

#include <SYSTEM/INPUT.H>
#include <SYSTEM/KEYBOARD.H>
#include <SYSTEM/KEYBOARD_KEYS.H>
#include <SYSTEM/TIMER.H>

S32 GamepadAnalogMove = LBA2_ANALOG_MOVE_DEFAULT;
S32 GamepadDpadZoom = LBA2_MODERN_PAD_DEFAULT;
S32 GamepadBehaviourCycle = LBA2_MODERN_PAD_DEFAULT;
S32 GamepadCameraCurve = LBA2_MODERN_PAD_DEFAULT;

static S32 s_cycleHeld = 0;
S32 GamepadQuickMenu = LBA2_MODERN_PAD_DEFAULT;
static S32 s_quickHeld = 0;

S32 AnalogMove_QuickMenuPressed(void) {
    S32 down;
    S32 pressed;
    if (!GamepadQuickMenu || !JoystickIsPresent() || Record_IsReplaying()) {
        s_quickHeld = 0;
        return 0;
    }
    down = CheckKey(K_GAMEPAD_RSHOULDER) ? 1 : 0;
    pressed = down && !s_quickHeld;
    s_quickHeld = down;
    return pressed;
}

S32 AnalogMove_BehaviourCyclePressed(void) {
    S32 down;
    S32 pressed;
    if (!GamepadBehaviourCycle || !JoystickIsPresent() || Record_IsReplaying()) {
        s_cycleHeld = 0;
        return 0;
    }
    down = CheckKey(K_GAMEPAD_LTRIGGER) ? 1 : 0;
    pressed = down && !s_cycleHeld;
    s_cycleHeld = down;
    return pressed;
}

S32 AnalogMove_DpadZoom(void) {
    if (!GamepadDpadZoom || !JoystickIsPresent() || Record_IsReplaying())
        return 0;
    if (CheckKey(K_GAMEPAD_DPAD_UP))
        return ANALOG_MOVE_DPAD_ZOOM_STEP;
    if (CheckKey(K_GAMEPAD_DPAD_DOWN))
        return -ANALOG_MOVE_DPAD_ZOOM_STEP;
    return 0;
}

/* The camera yaw the stick is read against while it stays held. A classic
 * exterior camera cuts when Twinsen leaves the screen, and the Auto camera
 * swings round behind him: read against the live yaw, either would turn him
 * under a stick that has not moved. The reference is taken again when the
 * stick is let go or swung to a new direction. */
static S32 s_refBetaCam = 0;
static S32 s_refStickAngle = 0;
static S32 s_held = 0;
static U32 s_lastTime = 0;

/* A swing past this re-reads the camera, so a new direction means the new view. */
#define ANALOG_MOVE_RELATCH_ANGLE 512
/* A frame longer than this (a pause, a load) turns him no further than this one. */
#define ANALOG_MOVE_MAX_DT_MS 100

/* A stick pulled this far against his facing backs him up near a ladder. */
#define ANALOG_MOVE_BACK_MIN_DIFF 1536

void AnalogMove_ApplyBuggy(void) {
    if (!GamepadAnalogMove || !JoystickIsPresent() || Record_IsReplaying())
        return;
    if (CheckKey(K_GAMEPAD_RTRIGGER))
        Input |= I_UP;
    else if (CheckKey(K_GAMEPAD_LTRIGGER))
        Input |= I_DOWN;
}

void AnalogMove_Apply(S32 *beta, S32 canTurn, S32 exterior, S32 onLadder, S32 nearLadder, S32 betaCam,
                      S32 cameraOrbiting) {
    S16 lx = 0;
    S16 ly = 0;
    S32 dz;
    S32 target;
    S32 diff;
    U32 now = TimerRefHR;
    U32 dt = now - s_lastTime;
    s_lastTime = now;
    if (dt > ANALOG_MOVE_MAX_DT_MS)
        dt = ANALOG_MOVE_MAX_DT_MS;

    /* A replay drives the hero from the recorded keys alone; the stick's raw
       axes are not in a recording. */
    if (!GamepadAnalogMove || !JoystickIsPresent() || Record_IsReplaying()) {
        s_held = 0;
        return;
    }

    JoystickGetLeftStick(&lx, &ly);
    dz = JoystickDeadzone;
    if ((S32)lx * lx + (S32)ly * ly < dz * dz) {
        s_held = 0;
        return;
    }

    /* On a ladder, up climbs and down comes down. */
    if (onLadder) {
        s_held = 0;
        Input &= ~(I_UP | I_DOWN);
        if (ly < -dz)
            Input |= I_UP;
        else if (ly > dz)
            Input |= I_DOWN;
        return;
    }

    if (exterior) {
        S32 stick = AnalogMove_StickAngle(lx, ly);
        S32 swing = AnalogMove_AngleDiff(s_refStickAngle, stick);
        if (swing < 0)
            swing = -swing;
        if (!s_held || swing > ANALOG_MOVE_RELATCH_ANGLE || cameraOrbiting) {
            s_refBetaCam = betaCam;
            s_refStickAngle = stick;
        }
        target = AnalogMove_TargetExterior(lx, ly, s_refBetaCam);
    } else {
        target = AnalogMove_TargetIso(lx, ly);
    }
    s_held = 1;

    /* The stick owns the frame's movement: no tank turn, no walking backwards. */
    Input &= ~I_JOY;

    /* At a ladder or window, pulling away from it backs him up (down the
       ladder from the top) rather than turning him round on the spot. */
    if (nearLadder) {
        diff = AnalogMove_AngleDiff(*beta, target);
        if (diff >= ANALOG_MOVE_BACK_MIN_DIFF || diff <= -ANALOG_MOVE_BACK_MIN_DIFF) {
            Input |= I_DOWN;
            return;
        }
    }

    if (canTurn) {
        S32 step = (S32)((ANALOG_MOVE_TURN_RATE * dt) / 1000);
        *beta = AnalogMove_StepToward(*beta, target, step);
    }
    diff = AnalogMove_AngleDiff(*beta, target);

    if (diff <= ANALOG_MOVE_WALK_MAX_DIFF && diff >= -ANALOG_MOVE_WALK_MAX_DIFF) {
        Input |= I_UP;
    }
}
