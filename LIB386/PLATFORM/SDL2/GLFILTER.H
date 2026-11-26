#pragma once

/* Shader present filters (GLFILTER.CPP). Built for the Switch, where SDL2
 * draws with its GLES2 renderer; a desktop build can ask for them with
 * -DLBA2_SDL2_GL_FILTERS=1 and SDL_RENDER_DRIVER=opengles2. */
#ifndef LBA2_SDL2_GL_FILTERS
#if defined(__SWITCH__)
#define LBA2_SDL2_GL_FILTERS 1
#else
#define LBA2_SDL2_GL_FILTERS 0
#endif
#endif

/* The programs, in the order the fragment shader's MODE numbers them. */
#define LBA2_GL_FILTER_PIXELART 0
#define LBA2_GL_FILTER_SMOOTH 1
#define LBA2_GL_FILTER_CRT 2
#define LBA2_GL_FILTER_SMOOTH_CRT 3
#define LBA2_GL_FILTER_COUNT 4

#if LBA2_SDL2_GL_FILTERS
/* Whether `renderer` is SDL's GLES2 renderer and the GL entry points loaded. */
bool SDL2_GLFilterAvailable(SDL_Renderer *renderer);
/* Drop what was learnt about `renderer`, before it is destroyed. */
void SDL2_GLFilterForget(SDL_Renderer *renderer);
/* Draw `texture` (srcW x srcH) into `area` of an output `outH` pixels tall with
 * program `filter`, flushing SDL's queue first and restoring the GL state SDL
 * relies on after. False, having drawn nothing, when it cannot. */
bool SDL2_GLFilterDraw(SDL_Renderer *renderer, SDL_Texture *texture, int filter, uint32_t srcW, uint32_t srcH,
                       const SDL_Rect *area, int outH);
#endif
