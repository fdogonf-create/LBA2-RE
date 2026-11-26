// ──────────────────────────────────────────────────────────────────────────
// PNG loading (using stb_image)
// ──────────────────────────────────────────────────────────────────────────
#include <SYSTEM.H>
#include <FILEIO/LOADPNG.H>

#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "stb_image.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

U32 *LoadPNG_ARGB(const char *filename, S32 *width, S32 *height) {
    int w = 0, h = 0, channels = 0;
    unsigned char *rgba = stbi_load(filename, &w, &h, &channels, 4);
    U32 *argb;
    size_t i, count;

    if (rgba == NULL)
        return NULL;
    count = (size_t)w * (size_t)h;
    argb = (U32 *)malloc(count * sizeof(U32));
    if (argb == NULL) {
        stbi_image_free(rgba);
        return NULL;
    }
    for (i = 0; i < count; i++) {
        const unsigned char *p = rgba + i * 4;
        argb[i] = ((U32)p[3] << 24) | ((U32)p[0] << 16) | ((U32)p[1] << 8) | (U32)p[2];
    }
    stbi_image_free(rgba);
    *width = (S32)w;
    *height = (S32)h;
    return argb;
}

void FreePNG(U32 *pixels) {
    free(pixels);
}
