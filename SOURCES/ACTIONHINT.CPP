#include "ACTIONHINT.H"

#include <PLATFORM/PLATFORM.H>

#include "ACTIONHINT_ICON.H"

#include <math.h>
#include <stddef.h>

/* The icon's button disc is this many of its pixels across; the rest is margin. */
#define ACTION_HINT_ICON_DISC 24

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

/* Bilinear sample of the icon at (u, v) in icon pixels, premultiplied: the
 * colour channels come back already multiplied by the alpha. */
static void SampleIcon(double u, double v, U32 *r, U32 *g, U32 *b, U32 *a) {
    S32 x0 = (S32)floor(u - 0.5), y0 = (S32)floor(v - 0.5);
    double fx = (u - 0.5) - x0, fy = (v - 0.5) - y0;
    double acc[4] = {0, 0, 0, 0};
    S32 k;

    for (k = 0; k < 4; k++) {
        S32 x = x0 + (k & 1), y = y0 + (k >> 1);
        double wgt = ((k & 1) ? fx : 1.0 - fx) * ((k >> 1) ? fy : 1.0 - fy);
        U32 c, ca;

        if (x < 0 OR y < 0 OR x >= ACTION_HINT_ICON_W OR y >= ACTION_HINT_ICON_H)
            continue;
        c = kActionHintIcon[y * ACTION_HINT_ICON_W + x];
        ca = c >> 24;
        acc[0] += wgt * ((c >> 16) & 0xFF) * ca / 255.0;
        acc[1] += wgt * ((c >> 8) & 0xFF) * ca / 255.0;
        acc[2] += wgt * (c & 0xFF) * ca / 255.0;
        acc[3] += wgt * ca;
    }
    *r = (U32)(acc[0] + 0.5);
    *g = (U32)(acc[1] + 0.5);
    *b = (U32)(acc[2] + 0.5);
    *a = (U32)(acc[3] + 0.5);
}

void ActionHint_Render(U32 *frameBuffer, U32 pitch, U32 width, U32 height, const U8 *palette_rgb) {
    S32 pitchPixels;
    S32 radius;
    S32 halo;
    S32 cx, cy;
    S32 x, y;
    S32 size, x0, y0;
    double scale;

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

    /* A faint white disc behind the button, so it stands out on dark
     * scenery, and white inside it for the letter. */
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
            /* The icon's letter is cut out of its disc: fill the disc white
             * underneath so the letter reads white on any scenery. */
            a = DiscCoverage(d, radius - 1) * 235 / 255;
            if (a)
                Blend(&frameBuffer[y * pitchPixels + x], 255, 255, 255, a);
        }
    }

    /* The button itself, scaled so the drawn disc of the icon is `radius`. */
    size = radius * 2 * ACTION_HINT_ICON_W / ACTION_HINT_ICON_DISC;
    scale = (double)ACTION_HINT_ICON_W / size;
    x0 = cx - size / 2;
    y0 = cy - size / 2;
    for (y = 0; y < size; y++) {
        S32 py = y0 + y;
        if (py < 0 OR py >= (S32)height)
            continue;
        for (x = 0; x < size; x++) {
            S32 px = x0 + x;
            U32 r, g, b, a, *pixel, ia;

            if (px < 0 OR px >= (S32)width)
                continue;
            SampleIcon((x + 0.5) * scale, (y + 0.5) * scale, &r, &g, &b, &a);
            if (!a)
                continue;
            pixel = &frameBuffer[py * pitchPixels + px];
            if (a > 255)
                a = 255;
            ia = 255 - a;
            r += ((*pixel >> 16) & 0xFF) * ia / 255;
            g += ((*pixel >> 8) & 0xFF) * ia / 255;
            b += (*pixel & 0xFF) * ia / 255;
            *pixel = 0xFF000000u | ((r > 255 ? 255 : r) << 16) | ((g > 255 ? 255 : g) << 8) | (b > 255 ? 255 : b);
        }
    }
}
