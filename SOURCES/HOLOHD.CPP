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
    if (s_argb == NULL)
        return;
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
