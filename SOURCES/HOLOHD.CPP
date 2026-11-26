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

void HoloHD_Begin(S32 entry, S32 x0, S32 y0, const U8 *palette) {
    char name[64];
    char path[ADELINE_MAX_PATH];
    S32 w = 0, h = 0;
    S32 y;

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
    /* The map as the frame shows it until something is drawn over it. */
    for (y = 0; y < IMAGE_AUTHORED_HEIGHT; y++)
        memcpy(s_key + (size_t)y * IMAGE_AUTHORED_WIDTH, (U8 *)Log + (size_t)(y0 + y) * ModeDesiredX + x0,
               IMAGE_AUTHORED_WIDTH);
    /* The palette the map fades in to, so a fade dims the picture too. */
    memcpy(s_keyPalette, palette, sizeof(s_keyPalette));

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
    SetPresentBackdrop(&s_backdrop);
    LogPrintf("holomap: redrawn map %s (%dx%d)\n", path, (int)w, (int)h);
}
