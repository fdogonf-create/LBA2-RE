#include "ACTIONHINT.H"

#include <PLATFORM/PLATFORM.H>

#include <math.h>
#include <stddef.h>

S32 GamepadActionHint = LBA2_ACTION_HINT_DEFAULT;

static U32 s_lastMs = 0;
static S32 s_noted = 0;

S32 ActionHint_InReach(S32 dx, S32 dz, S32 beta) {
    double dist;
    S32 toward;
    S32 diff;

    if (dx > ACTION_HINT_REACH OR dx<-ACTION_HINT_REACH OR dz> ACTION_HINT_REACH OR dz < -ACTION_HINT_REACH)
        return FALSE;
    dist = sqrt((double)dx * dx + (double)dz * dz);
    if (dist > ACTION_HINT_REACH)
        return FALSE;
    /* Right on top of him there is no direction to speak of. */
    if (dist < 256)
        return TRUE;
    /* Engine angles: 0 is +Z, 1024 is +X. */
    toward = (S32)(atan2((double)dx, (double)dz) * 2048.0 / 3.14159265358979) & 4095;
    diff = (toward - beta) & 4095;
    if (diff > 2048)
        diff -= 4096;
    if (diff < 0)
        diff = -diff;
    return diff <= ACTION_HINT_CONE;
}

void ActionHint_Note(U32 nowMs) {
    s_lastMs = nowMs;
    s_noted = 1;
}

S32 ActionHint_Visible(U32 nowMs) {
    return GamepadActionHint AND s_noted AND(U32)(nowMs - s_lastMs) <= ACTION_HINT_HOLD_MS;
}

/* Source-over blend of one colour onto an opaque ARGB pixel. */
static void Blend(U32 *pixel, U32 r, U32 g, U32 b, U32 a) {
    U32 src = *pixel;
    U32 ia = 255 - a;
    U32 sr = (src >> 16) & 0xFF;
    U32 sg = (src >> 8) & 0xFF;
    U32 sb = src & 0xFF;

    *pixel = 0xFF000000u | (((sr * ia + r * a) / 255) << 16) | (((sg * ia + g * a) / 255) << 8) | ((sb * ia + b * a) / 255);
}

/* Coverage of a disc of radius `radius` at a pixel whose centre is `d` away
 * from the disc's, with a one-pixel soft edge. 0..255. */
static U32 DiscCoverage(double d, double radius) {
    double c = radius + 0.5 - d;
    if (c <= 0.0)
        return 0;
    if (c >= 1.0)
        return 255;
    return (U32)(c * 255.0);
}

/* The letter, 5 wide and 7 high. */
static const char *const kGlyphA[7] = {"..#..", ".#.#.", "#...#", "#...#", "#####", "#...#", "#...#"};

void ActionHint_Render(U32 *frameBuffer, U32 pitch, U32 width, U32 height, const U8 *palette_rgb) {
    S32 pitchPixels;
    S32 radius;
    S32 halo;
    S32 cx, cy;
    S32 x, y;
    S32 cell, gx0, gy0;

    (void)palette_rgb;
    if (frameBuffer == NULL OR width == 0 OR height == 0)
        return;
    if (!ActionHint_Visible((U32)g_platform.system.ticks_ms()))
        return;

    pitchPixels = (S32)(pitch / sizeof(U32));
    radius = (S32)height / 30;
    if (radius < 7)
        radius = 7;
    halo = radius + (radius + 3) / 4;
    cx = (S32)width - halo - (S32)height / 40 - 1;
    cy = halo + (S32)height / 40;

    /* A faint white disc behind the button, so it stands out on dark scenery,
     * then the button itself, dark like the Switch's. */
    for (y = cy - halo - 1; y <= cy + halo + 1; y++) {
        if (y < 0 OR y >= (S32)height)
            continue;
        for (x = cx - halo - 1; x <= cx + halo + 1; x++) {
            double d;
            U32 a;

            if (x < 0 OR x >= (S32)width)
                continue;
            d = sqrt((double)(x - cx) * (x - cx) + (double)(y - cy) * (y - cy));
            a = DiscCoverage(d, halo) * 70 / 255;
            if (a)
                Blend(&frameBuffer[y * pitchPixels + x], 255, 255, 255, a);
            a = DiscCoverage(d, radius) * 225 / 255;
            if (a)
                Blend(&frameBuffer[y * pitchPixels + x], 40, 40, 44, a);
        }
    }

    /* The letter, scaled to fit inside the button. */
    cell = (radius * 2) / 11;
    if (cell < 1)
        cell = 1;
    gx0 = cx - (5 * cell) / 2;
    gy0 = cy - (7 * cell) / 2;
    for (y = 0; y < 7 * cell; y++) {
        S32 py = gy0 + y;
        if (py < 0 OR py >= (S32)height)
            continue;
        for (x = 0; x < 5 * cell; x++) {
            S32 px = gx0 + x;
            if (px < 0 OR px >= (S32)width)
                continue;
            if (kGlyphA[y / cell][x / cell] == '#')
                Blend(&frameBuffer[py * pitchPixels + px], 255, 255, 255, 255);
        }
    }
}
