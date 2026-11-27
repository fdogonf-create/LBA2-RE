// ──────────────────────────────────────────────────────────────────────────
// PNG loading (using stb_image), for full-colour pictures shown over the
// indexed frame (the holomap's redrawn maps).
// ──────────────────────────────────────────────────────────────────────────
#ifndef LOADPNG_H
#define LOADPNG_H

#include <SYSTEM.H>

/* The PNG at `filename` as 0xAARRGGBB pixels, row after row, `*width` by
 * `*height`; NULL, leaving both untouched, when there is no such file or it
 * cannot be read. Free it with FreePNG. */
extern U32 *LoadPNG_ARGB(const char *filename, S32 *width, S32 *height);
extern void FreePNG(U32 *pixels);

#endif // LOADPNG_H
