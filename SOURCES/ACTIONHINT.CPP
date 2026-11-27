#include "ACTIONHINT.H"

#include <PLATFORM/PLATFORM.H>

#include "ACTIONHINT_ICON.H"
#include "ACTIONHINT_LABEL.H"

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

static S32 s_label = ACTION_HINT_LABEL_NONE;
static U32 s_labelMs = 0;

void ActionHint_NoteLabel(U32 nowMs, S32 label) {
    ActionHint_Note(nowMs);
    s_label = label;
    s_labelMs = nowMs;
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

/* Bilinear sample of `icon` at (u, v) in icon pixels, premultiplied: the
 * colour channels come back already multiplied by the alpha. */
static void SampleIcon(const U32 *icon, double u, double v, U32 *r, U32 *g, U32 *b, U32 *a) {
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
        c = icon[y * ACTION_HINT_ICON_W + x];
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

/* A coverage mask (w x h) drawn `height` pixels tall from its left edge at
 * x0, centred on cy, in white with a dark shadow so it reads on any scene. */
static void DrawLabel(U32 *frameBuffer, S32 pitchPixels, S32 width, S32 fbHeight, const U8 *mask, S32 w, S32 h,
                      S32 x0, S32 cy, S32 height) {
    S32 outW = w * height / h;
    S32 y0 = cy - height / 2;
    S32 shadow = height / 14 + 1;
    S32 pass;
    S32 x, y;

    for (pass = 0; pass < 2; pass++) {
        S32 off = pass == 0 ? shadow : 0;
        U32 c = pass == 0 ? 0 : 255;
        U32 maxA = pass == 0 ? 170 : 255;
        for (y = 0; y < height; y++) {
            S32 py = y0 + y + off;
            double v = (y + 0.5) * h / height - 0.5;
            S32 sy = (S32)floor(v);
            double fy = v - sy;
            if (py < 0 OR py >= fbHeight)
                continue;
            for (x = 0; x < outW; x++) {
                S32 px = x0 + x + off;
                double u = (x + 0.5) * w / outW - 0.5;
                S32 sx = (S32)floor(u);
                double fx = u - sx;
                double acc = 0;
                S32 k;
                if (px < 0 OR px >= width)
                    continue;
                for (k = 0; k < 4; k++) {
                    S32 tx = sx + (k & 1), ty = sy + (k >> 1);
                    double wgt = ((k & 1) ? fx : 1.0 - fx) * ((k >> 1) ? fy : 1.0 - fy);
                    if (tx < 0 OR ty < 0 OR tx >= w OR ty >= h)
                        continue;
                    acc += wgt * mask[ty * w + tx];
                }
                if (acc >= 1.0)
                    Blend(&frameBuffer[py * pitchPixels + px], c, c, c, (U32)(acc * maxA / 255.0));
            }
        }
    }
}

/* `icon` scaled to `size` pixels square, centred on (cx, cy). */
static void DrawIcon(U32 *frameBuffer, S32 pitchPixels, S32 width, S32 height, const U32 *icon, S32 cx, S32 cy,
                     S32 size) {
    double scale = (double)ACTION_HINT_ICON_W / size;
    S32 x0 = cx - size / 2;
    S32 y0 = cy - size / 2;
    S32 x, y;

    for (y = 0; y < size; y++) {
        S32 py = y0 + y;
        if (py < 0 OR py >= height)
            continue;
        for (x = 0; x < size; x++) {
            S32 px = x0 + x;
            U32 r, g, b, a, *pixel, ia;

            if (px < 0 OR px >= width)
                continue;
            SampleIcon(icon, (x + 0.5) * scale, (y + 0.5) * scale, &r, &g, &b, &a);
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

static T_ACTION_HINT_PROMPT s_prompts[ACTION_HINT_MAX_PROMPTS];
static S32 s_promptCount = 0;

void ActionHint_SetPrompts(const T_ACTION_HINT_PROMPT *prompts, S32 count) {
    S32 n;
    if (count > ACTION_HINT_MAX_PROMPTS)
        count = ACTION_HINT_MAX_PROMPTS;
    for (n = 0; n < count; n++)
        s_prompts[n] = prompts[n];
    s_promptCount = count < 0 ? 0 : count;
}

static const U32 *PromptIcon(S32 which) {
    switch (which) {
    case ACTION_HINT_ICON_BUTTON_B:
        return kIconB;
    case ACTION_HINT_ICON_DPAD_H:
        return kIconDPAD_H;
    default:
        return kIconA;
    }
}

void ActionHint_Render(U32 *frameBuffer, U32 pitch, U32 width, U32 height, const U8 *palette_rgb) {
    S32 pitchPixels;
    S32 radius;
    S32 halo;
    S32 cx, cy;
    S32 x, y;
    S32 n;
    S32 climb;

    (void)palette_rgb;
    if (frameBuffer == NULL OR width == 0 OR height == 0)
        return;
    pitchPixels = (S32)(pitch / sizeof(U32));

    /* Button prompts a menu asked for, placed in its own canvas units. */
    for (n = 0; n < s_promptCount; n++) {
        const T_ACTION_HINT_PROMPT *pr = &s_prompts[n];
        if (pr->canvasW <= 0 OR pr->canvasH <= 0)
            continue;
        DrawIcon(frameBuffer, pitchPixels, (S32)width, (S32)height, PromptIcon(pr->icon),
                 (S32)((S64)pr->x * (S32)width / pr->canvasW), (S32)((S64)pr->y * (S32)height / pr->canvasH),
                 (S32)((S64)pr->size * (S32)height / pr->canvasH));
    }

    if (!ActionHint_Visible((U32)g_platform.system.ticks_ms()))
        return;

    climb = s_label != ACTION_HINT_LABEL_NONE AND(U32)(g_platform.system.ticks_ms() - s_labelMs) <= ACTION_HINT_HOLD_MS;
    radius = (S32)height / 30;
    if (radius < 7)
        radius = 7;
    halo = radius + (radius + 3) / 4;
    cx = (S32)width - halo - (S32)height / 40 - 1;
    cy = halo + (S32)height / 40;

    /* A word beside it moves the button left by the word's width. */
    if (climb) {
        const U8 *mask = s_label == ACTION_HINT_LABEL_CLIMB_ES ? kLabelClimbEs : kLabelClimbEn;
        S32 w = s_label == ACTION_HINT_LABEL_CLIMB_ES ? kLabelClimbEs_W : kLabelClimbEn_W;
        S32 textH = radius * 3 / 2;
        S32 textW = w * textH / ACTION_HINT_LABEL_H;
        cx -= textW + radius / 2;
        DrawLabel(frameBuffer, pitchPixels, (S32)width, (S32)height, mask, w, ACTION_HINT_LABEL_H,
                  cx + halo + radius / 3, cy, textH);
    }

    /* A faint white disc behind the button, so it stands out on dark scenery. */
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
        }
    }

    /* The button itself, scaled so the drawn disc of the icon is `radius`. */
    /* Climbing is done by pushing the stick up, not with A: show the stick. */
    DrawIcon(frameBuffer, pitchPixels, (S32)width, (S32)height, climb ? kIconStickUp : kIconA, cx, cy,
             radius * 2 * ACTION_HINT_ICON_W / ACTION_HINT_ICON_DISC);
}
