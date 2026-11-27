#include "CAMHUD.H"

#include "DIRECTORIES.H"

#include <PLATFORM/PLATFORM.H>
#include <SYSTEM/LIMITS.H>

#include <stdio.h>
#include <string.h>

#define CAM_HUD_HOLD_MS 300
#define CAM_HUD_LINES 4

static S32 s_enabled = -1;
static U32 s_lastMs = 0;
static S32 s_noted = 0;
static char s_lines[CAM_HUD_LINES][48];

/* 5x7 glyphs, one byte a row, bit 4 the leftmost column. */
static const U8 *Glyph(char c) {
    static const U8 digits[10][7] = {
        {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
        {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}, {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E},
        {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
        {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
        {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}};
    static const U8 letters[26][7] = {
        {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, /* A */
        {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, /* B */
        {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, /* C */
        {0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C}, /* D */
        {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, /* E */
        {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, /* F */
        {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, /* G */
        {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, /* H */
        {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, /* I */
        {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}, /* J */
        {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, /* K */
        {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, /* L */
        {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, /* M */
        {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}, /* N */
        {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, /* O */
        {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, /* P */
        {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, /* Q */
        {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, /* R */
        {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, /* S */
        {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, /* T */
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, /* U */
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, /* V */
        {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}, /* W */
        {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, /* X */
        {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, /* Y */
        {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}  /* Z */
    };
    static const U8 colon[7] = {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00};
    static const U8 dash[7] = {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};
    static const U8 lpar[7] = {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02};
    static const U8 rpar[7] = {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08};
    static const U8 blank[7] = {0, 0, 0, 0, 0, 0, 0};

    if (c >= '0' AND c <= '9')
        return digits[c - '0'];
    if (c >= 'a' AND c <= 'z')
        c = (char)(c - 'a' + 'A');
    if (c >= 'A' AND c <= 'Z')
        return letters[c - 'A'];
    if (c == ':')
        return colon;
    if (c == '-')
        return dash;
    if (c == '(')
        return lpar;
    if (c == ')')
        return rpar;
    return blank;
}

S32 CamHud_Enabled(void) {
    if (s_enabled < 0) {
        char path[ADELINE_MAX_PATH];
        FILE *f;
        GetResPath(path, ADELINE_MAX_PATH, "tester_camara.txt");
        f = fopen(path, "rb");
        s_enabled = (f != NULL);
        if (f != NULL)
            fclose(f);
    }
    return s_enabled;
}

void CamHud_Note(U32 nowMs, S32 alpha, S32 alphaMin, S32 alphaMax, S32 dist, S32 distMin, S32 distMax, S32 beta,
                 S32 island, S32 cube) {
    if (!CamHud_Enabled())
        return;
    s_lastMs = nowMs;
    s_noted = 1;
    snprintf(s_lines[0], sizeof(s_lines[0]), "ALTURA: %d (%d-%d)", (int)alpha, (int)alphaMin, (int)alphaMax);
    snprintf(s_lines[1], sizeof(s_lines[1]), "DISTANCIA: %d (%d-%d)", (int)dist, (int)distMin, (int)distMax);
    snprintf(s_lines[2], sizeof(s_lines[2]), "GIRO: %d (0-4095)", (int)(beta & 4095));
    snprintf(s_lines[3], sizeof(s_lines[3]), "ISLA: %d  ESCENA: %d", (int)island, (int)cube);
}

static void FillRect(U32 *fb, S32 pitchPixels, S32 w, S32 h, S32 x0, S32 y0, S32 x1, S32 y1, U32 argb, S32 blend) {
    S32 x, y;
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 > w)
        x1 = w;
    if (y1 > h)
        y1 = h;
    for (y = y0; y < y1; y++) {
        U32 *row = fb + (size_t)y * (size_t)pitchPixels;
        for (x = x0; x < x1; x++) {
            if (blend) {
                const U32 p = row[x];
                /* Half towards black: the frame stays readable under the box. */
                row[x] = 0xFF000000u | ((p >> 1) & 0x007F7F7Fu);
            } else {
                row[x] = argb;
            }
        }
    }
}

void CamHud_Render(U32 *frameBuffer, U32 pitch, U32 width, U32 height, const U8 *palette_rgb) {
    S32 pitchPixels, scale, n, i, gx, gy, row, col, longest = 0;
    S32 x0, y0, lineH;
    U32 now;

    (void)palette_rgb;
    if (frameBuffer == NULL OR width == 0 OR height == 0 OR !s_noted)
        return;
    now = (U32)g_platform.system.ticks_ms();
    if ((U32)(now - s_lastMs) > CAM_HUD_HOLD_MS)
        return;
    pitchPixels = (S32)(pitch / sizeof(U32));
    scale = (S32)height / 240;
    if (scale < 1)
        scale = 1;
    lineH = 9 * scale;
    x0 = 4 * scale;
    y0 = 4 * scale;
    for (n = 0; n < CAM_HUD_LINES; n++) {
        S32 len = (S32)strlen(s_lines[n]);
        if (len > longest)
            longest = len;
    }
    FillRect(frameBuffer, pitchPixels, (S32)width, (S32)height, x0 - 2 * scale, y0 - 2 * scale,
             x0 + longest * 6 * scale + 2 * scale, y0 + CAM_HUD_LINES * lineH + scale, 0, 1);
    for (n = 0; n < CAM_HUD_LINES; n++) {
        const char *s = s_lines[n];
        for (i = 0; s[i]; i++) {
            const U8 *g = Glyph(s[i]);
            gx = x0 + i * 6 * scale;
            gy = y0 + n * lineH;
            for (row = 0; row < 7; row++)
                for (col = 0; col < 5; col++)
                    if (g[row] & (0x10 >> col))
                        FillRect(frameBuffer, pitchPixels, (S32)width, (S32)height, gx + col * scale,
                                 gy + row * scale, gx + (col + 1) * scale, gy + (row + 1) * scale, 0xFFFFFF40u,
                                 0);
        }
    }
}
