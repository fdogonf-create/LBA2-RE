#include "HOLOHD.H"

#include "DIRECTORIES.H"
#include "IMAGE_LOAD.H"

#include <FILEIO/LOADPNG.H>
#include <SVGA/SCREEN.H>
#include <SVGA/VIDEO.H>
#include <SYSTEM/LOGPRINT.H>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PlatformBackdrop s_backdrop;
static U32 *s_argb = NULL;
static U8 *s_key = NULL;
static U8 s_keyPalette[768];
static U32 s_serial = 0;

void HoloHD_End(void) {
    SetPresentBackdrop(NULL);
    if (s_argb != NULL)
        FreePNG(s_argb);
    s_argb = NULL;
    free(s_key);
    s_key = NULL;
}

void HoloHD_SetView(S32 sx, S32 sy, S32 sw, S32 sh) {
    S32 y;

    if (s_argb == NULL OR s_key == NULL)
        return;
    /* The map as the frame shows it until something is drawn over it. */
    for (y = 0; y < IMAGE_AUTHORED_HEIGHT; y++)
        memcpy(s_key + (size_t)y * IMAGE_AUTHORED_WIDTH,
               (U8 *)Log + (size_t)(s_backdrop.y + y) * ModeDesiredX + s_backdrop.x, IMAGE_AUTHORED_WIDTH);
    /* The same part of the picture, in its own pixels. */
    s_backdrop.src_x = (S32)((S64)sx * (S32)s_backdrop.width / IMAGE_AUTHORED_WIDTH);
    s_backdrop.src_y = (S32)((S64)sy * (S32)s_backdrop.height / IMAGE_AUTHORED_HEIGHT);
    s_backdrop.src_w = (S32)((S64)sw * (S32)s_backdrop.width / IMAGE_AUTHORED_WIDTH);
    s_backdrop.src_h = (S32)((S64)sh * (S32)s_backdrop.height / IMAGE_AUTHORED_HEIGHT);
    SetPresentBackdrop(&s_backdrop);
}

void HoloHD_Begin(S32 entry, S32 x0, S32 y0, const U8 *palette) {
    char name[64];
    char path[ADELINE_MAX_PATH];
    S32 w = 0, h = 0;

    HoloHD_End();
    if (x0<0 OR y0<0 OR x0 + IMAGE_AUTHORED_WIDTH>(S32) ModeDesiredX
               OR y0 +
           IMAGE_AUTHORED_HEIGHT>(S32) ModeDesiredY)
        return;
    snprintf(name, sizeof(name), "hd/holomap_%d.png", (int)entry);
    GetResPath(path, ADELINE_MAX_PATH, name);
    s_argb = LoadPNG_ARGB(path, &w, &h);
    if (s_argb == NULL) {
        LogPrintf("holomap: no redrawn map at %s, showing the original\n", path);
        return;
    }
    s_key = (U8 *)malloc((size_t)IMAGE_AUTHORED_WIDTH * IMAGE_AUTHORED_HEIGHT);
    if (s_key == NULL) {
        HoloHD_End();
        return;
    }
    /* The palette the map fades in to, so a fade dims the picture too. */
    memcpy(s_keyPalette, palette, sizeof(s_keyPalette));

    memset(&s_backdrop, 0, sizeof(s_backdrop));
    s_backdrop.argb = s_argb;
    s_backdrop.width = (uint32_t)w;
    s_backdrop.height = (uint32_t)h;
    s_backdrop.x = x0;
    s_backdrop.y = y0;
    s_backdrop.w = IMAGE_AUTHORED_WIDTH;
    s_backdrop.h = IMAGE_AUTHORED_HEIGHT;
    s_backdrop.key = s_key;
    s_backdrop.key_palette = s_keyPalette;
    s_backdrop.serial = ++s_serial;
    LogPrintf("holomap: redrawn map %s (%dx%d)\n", path, (int)w, (int)h);
}

void HoloHD_ReplaceTexture(S32 entry, U8 *texture, U32 size, const U8 *palette) {
    char name[64];
    char path[ADELINE_MAX_PATH];
    S32 w = 0, h = 0, x, y, i;
    const S32 tw = 256;
    const S32 th = (S32)(size / 256);
    U8 used[256];
    U8 *cache;
    U32 *argb;

    if (texture == NULL OR palette == NULL OR th <= 0)
        return;
    snprintf(name, sizeof(name), "hd/hologlobe_%d.png", (int)entry);
    GetResPath(path, ADELINE_MAX_PATH, name);
    argb = LoadPNG_ARGB(path, &w, &h);
    if (argb == NULL)
        return;
    cache = (U8 *)malloc(32768 * 2); /* per 15-bit colour: found flag, index */
    if (cache == NULL) {
        FreePNG(argb);
        return;
    }
    memset(cache, 0, 32768 * 2);
    memset(used, 0, sizeof(used));
    for (i = 0; i < tw * th; i++)
        used[texture[i]] = 1;

    for (y = 0; y < th; y++) {
        const U32 *row = argb + (size_t)(y * h / th) * w;
        for (x = 0; x < tw; x++) {
            const U32 c = row[x * w / tw];
            const S32 r = (S32)((c >> 16) & 0xFF), g = (S32)((c >> 8) & 0xFF), b = (S32)(c & 0xFF);
            const S32 key = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
            if (!cache[key * 2]) {
                S32 best = 0, bestDist = 0x7FFFFFFF;
                for (i = 0; i < 256; i++) {
                    S32 dr, dg, db, dist;
                    if (!used[i])
                        continue;
                    dr = (S32)palette[i * 3] - r;
                    dg = (S32)palette[i * 3 + 1] - g;
                    db = (S32)palette[i * 3 + 2] - b;
                    dist = dr * dr * 3 + dg * dg * 4 + db * db * 2;
                    if (dist < bestDist) {
                        bestDist = dist;
                        best = i;
                    }
                }
                cache[key * 2] = 1;
                cache[key * 2 + 1] = (U8)best;
            }
            texture[y * tw + x] = cache[key * 2 + 1];
        }
    }
    free(cache);
    FreePNG(argb);
    LogPrintf("holomap: globe surface %d from %s (%dx%d)\n", (int)entry, path, (int)w, (int)h);
}
