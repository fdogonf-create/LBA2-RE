#include "INPUT_BINDINGS.H"

#include <string.h> // memcpy

#include <SYSTEM/INPUT.H>         // DefineInputKeys
#include <SYSTEM/KEYBOARD.H>      // CheckKey
#include <SYSTEM/KEYBOARD_KEYS.H> // K_*
#include "INPUT.H"                // I_RETURN, I_MENUS (the Switch menus)

// The 1997 source carried two default layouts, DefKeysDefault for DOS and this
// one for Windows 95. Only the second was ported. A binding set is where the
// first would come back if anyone wants it.
const T_DEF_KEY DefKeysDefault95[MAX_INPUT_SLOTS + 1] =
    {
        {K_GRAY_UP, K_NUMPAD_8},    // I_UP
        {K_GRAY_DOWN, K_NUMPAD_2},  // I_DOWN
        {K_GRAY_LEFT, K_NUMPAD_4},  // I_LEFT
        {K_GRAY_RIGHT, K_NUMPAD_6}, // I_RIGHT
        {K_ALT, 0},                 // I_THROW
        {K_CTRL, 0},                // I_COMPORTEMENT
        {K_SHIFT, 0},               // I_INVENTORY
        {K_SPACE, K_NUMPAD_5},      // I_ACTION_M
        {K_W, K_GRAY_END},          // I_ACTION_ALWAYS
        {K_ENTER, K_NUMPAD_ENTER},  // I_RETURN
        {K_F10, 0},                 // I_MENUS
        {K_H, K_TAB},               // I_HOLOMAP
        {K_P, 0},                   // I_PAUSE
        {K_X, 0},                   // I_ESQUIVE
        {K_F5, 0},                  // I_NORMAL
        {K_F6, 0},                  // I_SPORTIF
        {K_F7, 0},                  // I_AGRESSIF
        {K_F8, 0},                  // I_DISCRET
        {K_F1, 0},                  // I_HELP
        {K_F2, K_S},                // I_SAVE
        {K_F3, K_L},                // I_LOAD
        {K_F4, K_O},                // I_OPTIONS
        {K_BACKSPACE,
         0},                                // I_CAMERA (backtick used for console)
        {K_NUMPAD_PLUS, K_GRAY_PAGE_UP},    // I_CAMERA_LEVEL_PLUS
        {K_NUMPAD_MOINS, K_GRAY_PAGE_DOWN}, // I_CAMERA_LEVEL_MOINS
        {K_1, 0},                           // I_WEAPON_1
        {K_2, 0},                           // I_WEAPON_2
        {K_3, 0},                           // I_WEAPON_3
        {K_4, 0},                           // I_WEAPON_4
        {K_5, 0},                           // I_WEAPON_5
        {K_6, 0},                           // I_WEAPON_6
        {K_7, 0},                           // I_WEAPON_7
        {K_N, 0},                           // I_PINGOUIN
        {K_J, 0},                           // I_JETPACK
        {K_C, 0},                           // I_PROTECTION
        {K_F, 0},                           // I_FOUDRE
};

T_DEF_KEY MemoDefKeys[MAX_INPUT_SLOTS + 1];
T_DEF_KEY DefKeys[MAX_INPUT_SLOTS + 1];

// tableau de configuration des touches utilisé au cours du jeux
U32 TabInputBit[MAX_INPUT_SLOTS * 2 + 1];

// Combined keyboard+gamepad key/mask table handed to GetInput(). Folding the
// gamepad in here (instead of OR-ing it into Input afterward) makes it a
// first-class input: it flows through the one Input rebuild + NoRepeatInput
// edge-filter, so held pad buttons edge-detect like keyboard keys and survive a
// mid-frame GetInput() rebuild. Only the first 32 slots map to Input bits; four
// keys per slot (keyboard Key1/Key2 + gamepad Key1/Key2) => 32 * 4 entries.
#define INPUT_TABLE_SLOTS 32
static U32 CombinedInputKeys[INPUT_TABLE_SLOTS * 4];
static U32 CombinedInputMasks[INPUT_TABLE_SLOTS * 4];

T_DEF_KEY GamepadKeys[MAX_INPUT_SLOTS];

const T_DEF_KEY GamepadKeysDefault[MAX_INPUT_SLOTS] = {
#ifdef LBA2_GAMEPAD_STICK_LAYOUT
    {K_GAMEPAD_LSTICK_UP, 0},   // 0  I_UP
    {K_GAMEPAD_LSTICK_DOWN, 0}, // 1  I_DOWN
#elif defined(LBA2_MODERN_PAD_DEFAULT) && LBA2_MODERN_PAD_DEFAULT
    /* The Switch layout. SDL names the face buttons by position (Xbox style),
       so the Nintendo labels are swapped against the names here: K_GAMEPAD_B
       (east) is the Switch's A, K_GAMEPAD_A (south) its B, K_GAMEPAD_Y (north)
       its X and K_GAMEPAD_X (west) its Y. The left stick walks him (analog
       movement supplies I_UP), so neither trigger is forward or back. */
    {0, 0}, // 0  I_UP (the stick walks him; ZR casts the protection spell)
    {0, 0}, // 1  I_DOWN (ZL steps the behaviour; the stick turns him round)
#else
    {K_GAMEPAD_RTRIGGER, 0}, // 0  I_UP
    {K_GAMEPAD_LTRIGGER, 0}, // 1  I_DOWN
#endif
    {K_GAMEPAD_LSTICK_LEFT, 0},  // 2  I_LEFT
    {K_GAMEPAD_LSTICK_RIGHT, 0}, // 3  I_RIGHT
#if defined(LBA2_MODERN_PAD_DEFAULT) && LBA2_MODERN_PAD_DEFAULT && !defined(LBA2_GAMEPAD_STICK_LAYOUT)
    {K_GAMEPAD_A, 0},          // 4  I_THROW (Switch B; R opens the quick item menu)
    {K_GAMEPAD_LSHOULDER, 0},  // 5  I_COMPORTEMENT
    {K_GAMEPAD_BACK, 0},       // 6  I_INVENTORY (Switch -)
    {K_GAMEPAD_Y, 0},          // 7  I_ACTION_M (Switch X)
    {K_GAMEPAD_B, 0},          // 8  I_ACTION_ALWAYS (Switch A)
    {K_GAMEPAD_RSTICK_BTN, 0}, // 9  I_RETURN (right stick click; in menus the Switch A confirms)
    {K_GAMEPAD_START, 0},      // 10 I_MENUS
    {K_GAMEPAD_X, 0},          // 11 I_HOLOMAP (Switch Y)
    {0, 0},                    // 12 I_PAUSE (unbound; + opens the in-game menu)
    {K_GAMEPAD_LSTICK_BTN, 0}, // 13 I_ESQUIVE
#else
    {K_GAMEPAD_RSHOULDER, 0}, // 4  I_THROW
    {K_GAMEPAD_LSHOULDER, 0}, // 5  I_COMPORTEMENT
    {K_GAMEPAD_Y, 0},         // 6  I_INVENTORY
    {K_GAMEPAD_X, 0},         // 7  I_ACTION_M
    {K_GAMEPAD_A, 0},         // 8  I_ACTION_ALWAYS
#ifdef LBA2_GAMEPAD_STICK_LAYOUT
    {0, 0}, // 9  I_RETURN (front-screen tap)
#else
    {K_GAMEPAD_B, 0}, // 9  I_RETURN
#endif
    {K_GAMEPAD_START, 0}, // 10 I_MENUS
    {K_GAMEPAD_BACK, 0},  // 11 I_HOLOMAP
    {0, 0},               // 12 I_PAUSE (unbound; Start opens the in-game menu)
#ifdef LBA2_GAMEPAD_STICK_LAYOUT
    {K_GAMEPAD_B, 0}, // 13 I_ESQUIVE
#else
    {K_GAMEPAD_LSTICK_BTN, K_GAMEPAD_RSTICK_BTN}, // 13 I_ESQUIVE
#endif
#endif
    {0, 0},                                          // 14 I_NORMAL
    {0, 0},                                          // 15 I_SPORTIF
    {0, 0},                                          // 16 I_AGRESSIF
    {0, 0},                                          // 17 I_DISCRET
    {0, 0},                                          // 18 I_HELP
    {0, 0},                                          // 19 I_SAVE
    {0, 0},                                          // 20 I_LOAD
    {0, 0},                                          // 21 I_OPTIONS
    {K_GAMEPAD_RSTICK_LEFT, K_GAMEPAD_RSTICK_RIGHT}, // 22 I_CAMERA (classic-mode turn; analog orbit owns Auto camera)
    {K_GAMEPAD_RSTICK_UP, 0},                        // 23 I_CAMERA_LEVEL_PLUS
    {K_GAMEPAD_RSTICK_DOWN, 0},                      // 24 I_CAMERA_LEVEL_MOINS
    {0, 0},                                          // 25 I_WEAPON_1
    {0, 0},                                          // 26
    {0, 0},                                          // 27
    {0, 0},                                          // 28
    {0, 0},                                          // 29
    {0, 0},                                          // 30
    {0, 0},                                          // 31
    {K_GAMEPAD_DPAD_RIGHT, 0},                       // 32 I_PINGOUIN
#if defined(LBA2_MODERN_PAD_DEFAULT) && LBA2_MODERN_PAD_DEFAULT
    {0, 0},                  // 33 I_JETPACK (D-pad down zooms the camera out; the behaviour menu has it)
    {K_GAMEPAD_RTRIGGER, 0}, // 34 I_PROTECTION (ZR; D-pad up zooms the camera in)
#else
    {K_GAMEPAD_DPAD_DOWN, 0}, // 33 I_JETPACK
    {K_GAMEPAD_DPAD_UP, 0},   // 34 I_PROTECTION
#endif
    {K_GAMEPAD_DPAD_LEFT, 0}, // 35 I_FOUDRE
};

U32 NbInput = MAX_INPUT_SLOTS;

#ifdef LBA2_GAMEPAD_STICK_LAYOUT
static S32 s_modalCircle = 1;
#else
static S32 s_modalCircle = 0;
#endif
static S32 s_gameplay = 0;
static S32 s_circleSuppressed = 0;

#if defined(LBA2_MODERN_PAD_DEFAULT) && LBA2_MODERN_PAD_DEFAULT && !defined(LBA2_GAMEPAD_STICK_LAYOUT)
/* The Switch layout's menus: outside gameplay the Switch A (K_GAMEPAD_B)
   confirms, as Return does, and the Switch B (K_GAMEPAD_A) goes back, as
   Escape does, whatever either is bound to in play. A button still held across
   the change does nothing until it is released, so the press that opened a
   menu or a dialogue does not also answer it. */
#define BINDINGS_SWITCH_MENUS 1
static S32 s_switchGameplay = 0;
static U32 s_switchHeld = 0; /* bit 0: Switch A, bit 1: Switch B */

static U32 SwitchMenuMask(U32 key, U32 mask) {
    if (s_switchGameplay)
        return mask;
    if (key == K_GAMEPAD_B)
        return (s_switchHeld & 1u) ? 0 : I_RETURN;
    if (key == K_GAMEPAD_A)
        return (s_switchHeld & 2u) ? 0 : I_MENUS;
    return mask;
}
#endif

U32 Bindings_GetContext(void) {
    return (s_modalCircle ? BINDINGS_CONTEXT_MODAL_CIRCLE : 0) |
           (s_gameplay ? BINDINGS_CONTEXT_GAMEPLAY : 0) |
           (s_circleSuppressed ? BINDINGS_CONTEXT_CIRCLE_SUPPRESSED : 0);
}

void Bindings_SetContext(U32 context) {
    s_modalCircle = (context & BINDINGS_CONTEXT_MODAL_CIRCLE) != 0;
    s_gameplay = (context & BINDINGS_CONTEXT_GAMEPLAY) != 0;
    s_circleSuppressed = (context & BINDINGS_CONTEXT_CIRCLE_SUPPRESSED) != 0;
    InitInput();
}

//-------------------------------------------------------------------------
void InitInput(void) {
    // Rebuild the combined table from the current keyboard (DefKeys) and gamepad
    // (GamepadKeys) bindings. Call after either changes (key remap, config load,
    // restore-defaults). Spell slots 32-35 stay out of the Input path and are
    // read via direct CheckKey(DefKeys[..]) in PERSO.CPP.
    S32 n = 0;
    for (S32 i = 0; i < INPUT_TABLE_SLOTS; i++) {
        U32 mask = (1u << i);
        CombinedInputKeys[n] = DefKeys[i].Key1;
        CombinedInputMasks[n++] = mask;
        CombinedInputKeys[n] = DefKeys[i].Key2;
        CombinedInputMasks[n++] = mask;
        CombinedInputKeys[n] = GamepadKeys[i].Key1;
        CombinedInputMasks[n++] = mask;
        CombinedInputKeys[n] = GamepadKeys[i].Key2;
        CombinedInputMasks[n++] = mask;
#ifdef BINDINGS_SWITCH_MENUS
        CombinedInputMasks[n - 2] = SwitchMenuMask(CombinedInputKeys[n - 2], mask);
        CombinedInputMasks[n - 1] = SwitchMenuMask(CombinedInputKeys[n - 1], mask);
#endif
        /* An action remap must not turn a held modal press into a new action. */
        if (s_modalCircle && i == 13) {
            for (S32 key = n - 2; key < n; ++key)
                if (CombinedInputKeys[key] == K_GAMEPAD_B) {
                    if (s_circleSuppressed)
                        CombinedInputMasks[key] = 0;
                    else if (!s_gameplay)
                        CombinedInputMasks[key] = 1u << 9;
                }
        }
    }

    NbInput = (U32)n;

    DefineInputKeys(n, CombinedInputKeys, CombinedInputMasks);
}

void Bindings_SetGameplay(S32 gameplay) {
#ifdef BINDINGS_SWITCH_MENUS
    {
        U32 held = s_switchHeld;
        gameplay = gameplay != 0;
        /* Released buttons rearm; one held through a change of context waits. */
        if (!CheckKey(K_GAMEPAD_B))
            held &= ~1u;
        else if (s_switchGameplay != gameplay)
            held |= 1u;
        if (!CheckKey(K_GAMEPAD_A))
            held &= ~2u;
        else if (s_switchGameplay != gameplay)
            held |= 2u;
        if (s_switchGameplay != gameplay || s_switchHeld != held) {
            s_switchGameplay = gameplay;
            s_switchHeld = held;
            InitInput();
        }
    }
#endif
    if (!s_modalCircle)
        return;
    gameplay = gameplay != 0;
    S32 suppressed = s_circleSuppressed;
    if (!CheckKey(K_GAMEPAD_B))
        suppressed = 0;
    else if (s_gameplay != gameplay)
        suppressed = 1;
    if (s_gameplay != gameplay || s_circleSuppressed != suppressed) {
        s_gameplay = gameplay;
        s_circleSuppressed = suppressed;
        InitInput();
    }
}

//-------------------------------------------------------------------------
// The reverse of the fold above. Pure lookups over the two tables, in the same
// order InitInput reads them, so a key this hands back is one the forward
// direction would have folded into that slot's mask.
S32 Bindings_KeysForSlot(S32 slot, U32 *out, S32 max) {
    S32 n = 0;

    if (slot < 0 OR slot >= MAX_INPUT_SLOTS OR out == NULL)
        return 0;

    const U32 candidates[4] = {DefKeys[slot].Key1, DefKeys[slot].Key2,
                               GamepadKeys[slot].Key1, GamepadKeys[slot].Key2};
    for (S32 i = 0; i < 4 AND n < max; i++) {
        if (candidates[i] != 0)
            out[n++] = candidates[i];
    }
    return n;
}

//-------------------------------------------------------------------------
S32 Bindings_SlotForKey(U32 key) {
    // Zero is the unbound marker in both tables, not a key. Answering the
    // question for it would report every empty cell as a match.
    if (key == 0)
        return -1;

    // Through the same helper rather than a second copy of the four-cell read,
    // so the two directions cannot drift apart on which cells count.
    for (S32 i = 0; i < MAX_INPUT_SLOTS; i++) {
        U32 keys[4];
        S32 n = Bindings_KeysForSlot(i, keys, 4);
        for (S32 k = 0; k < n; k++) {
            if (keys[k] == key)
                return i;
        }
    }
    return -1;
}

//-------------------------------------------------------------------------
S32 Bindings_CheckReversible(S32 *badSlot, U32 *badKey) {
    // Unbound first: a slot with no key is the answer to "why did this action
    // not come back", and reporting a sharing pair from further down the table
    // would bury it.
    for (S32 i = 0; i < MAX_INPUT_SLOTS; i++) {
        U32 keys[4];
        if (Bindings_KeysForSlot(i, keys, 4) == 0) {
            if (badSlot)
                *badSlot = i;
            if (badKey)
                *badKey = 0;
            return BINDINGS_REVERSE_UNBOUND;
        }
    }

    for (S32 i = 0; i < MAX_INPUT_SLOTS; i++) {
        U32 keys[4];
        S32 n = Bindings_KeysForSlot(i, keys, 4);
        for (S32 k = 0; k < n; k++) {
            // The lookup scans in slot order, so a key names the lowest slot
            // that binds it. A key of this slot naming a different one is the
            // sharing case, and it is this slot that cannot be expressed.
            S32 owner = Bindings_SlotForKey(keys[k]);
            if (owner != i) {
                if (badSlot)
                    *badSlot = i;
                if (badKey)
                    *badKey = keys[k];
                return BINDINGS_REVERSE_SHARED;
            }
        }
    }

    return BINDINGS_REVERSE_OK;
}

//-------------------------------------------------------------------------
void RestoreInput() {
    const T_DEF_KEY *DEFKEYSDEFAULT;
    DEFKEYSDEFAULT = DefKeysDefault95;

    memcpy(DefKeys, DEFKEYSDEFAULT, sizeof(T_DEF_KEY) * MAX_INPUT_SLOTS);
    memcpy(GamepadKeys, GamepadKeysDefault, sizeof(GamepadKeys));
}
