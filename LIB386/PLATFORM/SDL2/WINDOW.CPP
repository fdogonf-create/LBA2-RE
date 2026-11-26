#include "PRIVATE.H"

#include "GLFILTER.H"

#include <limits.h>

namespace {
bool s_initialized = false;
const char *s_title = NULL;
SDL_Window *s_window = NULL;
SDL_Renderer *s_renderer = NULL;
SDL_Texture *s_texture = NULL;
SDL_Texture *s_videoTexture = NULL; /* movie frames (IYUV), sized to the source */
uint32_t s_videoWidth = 0, s_videoHeight = 0;
uint32_t s_width = 0, s_height = 0;
bool s_fullscreen = false;
SDL_ScaleMode s_scaleMode = SDL_ScaleModeNearest;
int s_glFilter = -1; /* LBA2_GL_FILTER_*, or -1 for SDL's own copy */
/* The last frame's backdrop (PlatformBackdrop), drawn under the frame. */
SDL_Texture *s_backdropTexture = NULL;
const uint32_t *s_backdropPixels = NULL;
uint32_t s_backdropSerial = 0;
bool s_backdropOn = false;
SDL_Rect s_backdropRect = {0, 0, 0, 0}; /* in frame pixels */
SDL_Rect s_backdropSrc = {0, 0, 0, 0};  /* in picture pixels; w 0 for all of it */
Uint8 s_backdropBright = 255;
#ifdef LBA2_SDL2_TESTS
void (*s_presentProbe)(SDL_Renderer *) = NULL;
int (*s_fullscreenProbe)(SDL_Window *, Uint32) = NULL;
#endif

bool Error(const char *operation) {
    Platform_Log(PLATFORM_LOG_ERROR, "%s: %s", operation, SDL_GetError());
    return false;
}
bool Dimensions(uint32_t width, uint32_t height) {
    if (!width || !height || width > INT_MAX / sizeof(uint32_t) || height > INT_MAX) {
        SDL_SetError("Invalid presentation dimensions");
        return Error("SDL2 surface");
    }
    return true;
}
SDL_Rect Viewport(int width, int height) {
    SDL_Rect area = {0, 0, width, height};
    if ((double)width * s_height > (double)height * s_width) {
        area.w = (int)((double)height * s_width / s_height);
        area.x = (width - area.w) / 2;
    } else {
        area.h = (int)((double)width * s_height / s_width);
        area.y = (height - area.h) / 2;
    }
    return area;
}
int32_t Coordinate(int32_t value, int origin, int extent, uint32_t dimension) {
    const double mapped = ((double)value - origin) * dimension / extent;
    if (mapped <= 0)
        return 0;
    if (mapped >= dimension)
        return (int32_t)dimension - 1;
    return (int32_t)mapped;
}
/* The frame's own viewport `area` narrowed to the frame-pixel rectangle r. */
SDL_Rect MapRect(const SDL_Rect &area, const SDL_Rect &r) {
    SDL_Rect out;
    out.x = area.x + (int)((double)r.x * area.w / s_width);
    out.y = area.y + (int)((double)r.y * area.h / s_height);
    out.w = (int)((double)(r.x + r.w) * area.w / s_width) + area.x - out.x;
    out.h = (int)((double)(r.y + r.h) * area.h / s_height) + area.y - out.y;
    return out;
}

/* Take the frame's backdrop: upload its picture when it is a new one, and
   clear the frame's alpha where the frame still shows the indexed original,
   so the picture shows through there. */
void ApplyBackdrop(const PlatformFrame *frame, uint32_t *pixels, uint32_t pitch) {
    const PlatformBackdrop *b = frame->backdrop;
    s_backdropOn = false;
    if (!b || !b->argb || !b->key || !b->width || !b->height || b->w <= 0 || b->h <= 0)
        return;
    if (!s_backdropTexture || b->argb != s_backdropPixels || b->serial != s_backdropSerial) {
        int tw = 0, th = 0;
        if (s_backdropTexture)
            SDL_QueryTexture(s_backdropTexture, NULL, NULL, &tw, &th);
        if (!s_backdropTexture || tw != (int)b->width || th != (int)b->height) {
            SDL_DestroyTexture(s_backdropTexture);
            s_backdropTexture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
                                                  (int)b->width, (int)b->height);
            if (!s_backdropTexture) {
                Error("Unable to create SDL2 backdrop texture");
                s_backdropPixels = NULL;
                return;
            }
            SDL_SetTextureScaleMode(s_backdropTexture, SDL_ScaleModeLinear);
            SDL_SetTextureBlendMode(s_backdropTexture, SDL_BLENDMODE_NONE);
        }
        if (SDL_UpdateTexture(s_backdropTexture, NULL, b->argb, (int)(b->width * sizeof(uint32_t))) != 0) {
            Error("Unable to upload SDL2 backdrop");
            s_backdropPixels = NULL;
            return;
        }
        s_backdropPixels = b->argb;
        s_backdropSerial = b->serial;
    }

    /* Dimmed with the palette: a fade takes the picture down with it. */
    uint32_t now = 0, full = 0;
    if (b->key_palette) {
        for (int i = 0; i < 256 * 3; ++i) {
            now += frame->palette_rgb[i];
            full += b->key_palette[i];
        }
    }
    s_backdropBright = (Uint8)(full == 0 || now >= full ? 255 : (now * 255) / full);

    const int x0 = b->x < 0 ? 0 : b->x, y0 = b->y < 0 ? 0 : b->y;
    const int x1 = b->x + b->w > (int)frame->width ? (int)frame->width : b->x + b->w;
    const int y1 = b->y + b->h > (int)frame->height ? (int)frame->height : b->y + b->h;
    for (int y = y0; y < y1; ++y) {
        const uint8_t *src = frame->pixels + (size_t)y * frame->pitch;
        const uint8_t *key = b->key + (size_t)(y - b->y) * (size_t)b->w - b->x;
        uint32_t *dst = (uint32_t *)((uint8_t *)pixels + (size_t)y * pitch);
        for (int x = x0; x < x1; ++x) {
            if (src[x] == key[x])
                dst[x] &= 0x00FFFFFFu;
        }
    }
    s_backdropRect.x = b->x;
    s_backdropRect.y = b->y;
    s_backdropRect.w = b->w;
    s_backdropRect.h = b->h;
    s_backdropSrc.x = b->src_x;
    s_backdropSrc.y = b->src_y;
    s_backdropSrc.w = b->src_w > 0 && b->src_h > 0 ? b->src_w : 0;
    s_backdropSrc.h = b->src_h;
    s_backdropOn = true;
}

bool PresentTexture() {
    int width, height;
    if (SDL_GetRendererOutputSize(s_renderer, &width, &height) != 0)
        return Error("Unable to query SDL2 render dimensions");
    const SDL_Rect area = Viewport(width, height);
    if (SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255) != 0 || SDL_RenderClear(s_renderer) != 0)
        return Error("Unable to present SDL2 frame");
    bool drawn = false;
    if (s_backdropOn && s_backdropTexture) {
        /* The picture first, the frame over it with its holes; a shader
           filter would have no use on a still map, so SDL's copy draws it. */
        const SDL_Rect dst = MapRect(area, s_backdropRect);
        SDL_SetTextureColorMod(s_backdropTexture, s_backdropBright, s_backdropBright, s_backdropBright);
        SDL_SetTextureBlendMode(s_texture, SDL_BLENDMODE_BLEND);
        if (SDL_RenderCopy(s_renderer, s_backdropTexture, s_backdropSrc.w ? &s_backdropSrc : NULL, &dst) != 0 ||
            SDL_RenderCopy(s_renderer, s_texture, NULL, &area) != 0)
            return Error("Unable to present SDL2 frame");
        drawn = true;
    } else {
        SDL_SetTextureBlendMode(s_texture, SDL_BLENDMODE_NONE);
    }
#if LBA2_SDL2_GL_FILTERS
    /* A shader filter that cannot draw (no GLES2, a shader the driver refused)
       falls back to the plain linear copy its texture is set up for. */
    if (!drawn && s_glFilter >= 0)
        drawn = SDL2_GLFilterDraw(s_renderer, s_texture, s_glFilter, s_width, s_height, &area, height);
#endif
    if (!drawn && SDL_RenderCopy(s_renderer, s_texture, NULL, &area) != 0)
        return Error("Unable to present SDL2 frame");
#ifdef LBA2_SDL2_TESTS
    if (s_presentProbe)
        s_presentProbe(s_renderer);
#endif
    SDL_RenderPresent(s_renderer);
    return true;
}
bool DisplaySize(int display, uint32_t *width, uint32_t *height) {
    SDL_DisplayMode mode;
    if (!width || !height || display < 0 || SDL_GetCurrentDisplayMode(display, &mode) != 0 ||
        mode.w <= 0 || mode.h <= 0)
        return false;
    *width = (uint32_t)mode.w;
    *height = (uint32_t)mode.h;
    return true;
}
} // namespace

#ifdef LBA2_SDL2_TESTS
SDL_Window *SDL2_TestWindow() { return s_window; }
void SDL2_SetPresentationProbe(void (*probe)(SDL_Renderer *)) { s_presentProbe = probe; }
void SDL2_SetFullscreenProbe(int (*probe)(SDL_Window *, Uint32)) { s_fullscreenProbe = probe; }
#endif

bool SDL2_InitWindow(const char *title) {
    if (!title) {
        SDL_SetError("Window title is null");
        return Error("Unable to initialize SDL2 window");
    }
    if (!SDL2_InitVideo())
        return Error("Unable to initialize SDL2 video");
    s_title = title;
    s_initialized = true;
    return true;
}
void SDL2_EndWindow() {
    if (s_initialized) {
        SDL2_DestroyWindow();
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        s_initialized = false;
    }
}
bool SDL2_WindowInitialized() { return s_initialized; }
bool SDL2_CreateWindow(uint32_t width, uint32_t height, bool fullscreen) {
    if (!s_initialized || s_window || !Dimensions(width, height)) {
        SDL_SetError("Window is not ready for surface creation");
        return Error("SDL2 create");
    }
    int nativeWidth = (int)width, nativeHeight = (int)height;
    Uint32 flags = SDL_WINDOW_RESIZABLE | (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
#ifdef __SWITCH__
    /* Switch needs EGL before window creation; SDL's software renderer cannot present there. */
    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) != 0 ||
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) != 0 ||
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0) != 0)
        return Error("Unable to configure Switch GLES");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
    SDL2_SwitchWindowSize(&nativeWidth, &nativeHeight);
    flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    fullscreen = true;
#elif defined(__vita__)
    if (!SDL2_VitaConfigureWindow(&nativeWidth, &nativeHeight))
        return false;
    flags = SDL_WINDOW_SHOWN;
    fullscreen = true;
#endif
    s_window = SDL_CreateWindow(s_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                nativeWidth, nativeHeight, flags);
    if (!s_window)
        return Error("Unable to create SDL2 window");
    s_fullscreen = fullscreen;
    s_renderer = SDL_CreateRenderer(s_window, -1, 0);
    if (!s_renderer) {
        SDL2_DestroyWindow();
        return Error("Unable to create SDL2 renderer");
    }
    SDL2_SetVSync(true);
    if (!SDL2_ResizeWindow(width, height, true)) {
        SDL2_DestroyWindow();
        return false;
    }
    SDL_ShowCursor(SDL_DISABLE);
    return true;
}
bool SDL2_ResizeWindow(uint32_t width, uint32_t height, bool keepSize) {
    if (!s_window || !s_renderer || !Dimensions(width, height))
        return false;
    SDL_Texture *texture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING, (int)width, (int)height);
    if (!texture)
        return Error("Unable to allocate SDL2 presentation texture");
    if (SDL_SetTextureScaleMode(texture, s_scaleMode) != 0) {
        SDL_DestroyTexture(texture);
        return Error("Unable to configure SDL2 logical presentation");
    }
    SDL_DestroyTexture(s_texture);
    s_texture = texture;
    s_width = width;
    s_height = height;
    if (!keepSize && g_platform.capabilities.windowed)
        SDL_SetWindowSize(s_window, (int)width, (int)height);
    return true;
}
void SDL2_DestroyWindow() {
    SDL_DestroyTexture(s_backdropTexture);
    s_backdropTexture = NULL;
    s_backdropPixels = NULL;
    s_backdropOn = false;
    SDL_DestroyTexture(s_videoTexture);
    s_videoTexture = NULL;
    s_videoWidth = s_videoHeight = 0;
    SDL_DestroyTexture(s_texture);
#if LBA2_SDL2_GL_FILTERS
    SDL2_GLFilterForget(s_renderer);
#endif
    SDL_DestroyRenderer(s_renderer);
    SDL_DestroyWindow(s_window);
    s_texture = NULL;
    s_renderer = NULL;
    s_window = NULL;
    s_width = s_height = 0;
}
bool SDL2_WindowSize(int32_t *width, int32_t *height) {
    if (!s_window || !width || !height)
        return false;
    int w, h;
    SDL_GetWindowSize(s_window, &w, &h);
    *width = w;
    *height = h;
    return true;
}
bool SDL2_DisplaySize(uint32_t *width, uint32_t *height) {
    return s_window && DisplaySize(SDL_GetWindowDisplayIndex(s_window), width, height);
}
bool SDL2_PrimaryDisplaySize(uint32_t *width, uint32_t *height) {
    const char *driver = SDL_GetCurrentVideoDriver();
    return driver && SDL_strcmp(driver, "dummy") && SDL_strcmp(driver, "offscreen") &&
           DisplaySize(0, width, height);
}
void SDL2_SurfaceCoords(int32_t x, int32_t y, int32_t *sx, int32_t *sy) {
    if (!s_renderer || !s_width || !s_height) {
        *sx = x;
        *sy = y;
        return;
    }
    /* SDL_RenderSetLogicalSize rewrites native mouse and finger events.
       Explicit letterboxing preserves the contract's window-pixel coordinates. */
    int width, height;
    SDL_GetWindowSize(s_window, &width, &height);
    const SDL_Rect area = Viewport(width, height);
    if (area.w <= 0 || area.h <= 0) {
        *sx = *sy = 0;
        return;
    }
    *sx = Coordinate(x, area.x, area.w, s_width);
    *sy = Coordinate(y, area.y, area.h, s_height);
}
void SDL2_SetFullscreen(bool enabled) {
    if (!g_platform.capabilities.windowed) {
        s_fullscreen = true;
        return;
    }
    const bool previous = s_fullscreen;
    s_fullscreen = enabled;
    if (!s_window)
        return;
    const Uint32 flags = enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    int result;
#ifdef LBA2_SDL2_TESTS
    if (s_fullscreenProbe)
        result = s_fullscreenProbe(s_window, flags);
    else
#endif
        result = SDL_SetWindowFullscreen(s_window, flags);
    if (result != 0) {
        s_fullscreen = previous;
        Platform_Log(PLATFORM_LOG_WARN, "Unable to change SDL2 fullscreen state: %s", SDL_GetError());
    } else if (s_renderer && s_texture)
        PresentTexture();
}
bool SDL2_GetFullscreen() { return s_fullscreen; }
bool SDL2_IsFullscreen() {
    return s_window && (!g_platform.capabilities.windowed ||
                        (SDL_GetWindowFlags(s_window) & SDL_WINDOW_FULLSCREEN) != 0);
}
void SDL2_SetVSync(bool enabled) {
    if (s_renderer && SDL_RenderSetVSync(s_renderer, enabled ? 1 : 0) != 0)
        Platform_Log(PLATFORM_LOG_WARN, "Unable to change SDL2 vsync: %s", SDL_GetError());
}
bool SDL2_SetPresentFilter(PlatformPresentFilter mode) {
    SDL_ScaleMode resolved;
    int glFilter = -1;
    switch (mode) {
    case PLATFORM_FILTER_NEAREST:
        resolved = SDL_ScaleModeNearest;
        break;
    case PLATFORM_FILTER_LINEAR:
        resolved = SDL_ScaleModeLinear;
        break;
    case PLATFORM_FILTER_PIXELART:
    case PLATFORM_FILTER_SMOOTH:
    case PLATFORM_FILTER_CRT:
    case PLATFORM_FILTER_SMOOTH_CRT:
#if LBA2_SDL2_GL_FILTERS
        /* Before the window exists the renderer cannot be asked; the filter is
           taken, and the first present falls back to linear if it cannot draw. */
        if (s_renderer && !SDL2_GLFilterAvailable(s_renderer)) {
            SDL_SetError("The %s present filter needs SDL's opengles2 renderer",
                         mode == PLATFORM_FILTER_PIXELART ? "pixelart" : "shader");
            return false;
        }
        resolved = SDL_ScaleModeLinear;
        glFilter = mode == PLATFORM_FILTER_PIXELART ? LBA2_GL_FILTER_PIXELART
                   : mode == PLATFORM_FILTER_SMOOTH ? LBA2_GL_FILTER_SMOOTH
                   : mode == PLATFORM_FILTER_CRT    ? LBA2_GL_FILTER_CRT
                                                    : LBA2_GL_FILTER_SMOOTH_CRT;
        break;
#else
        SDL_SetError("SDL2 does not support the %s present filter",
                     mode == PLATFORM_FILTER_PIXELART ? "pixelart" : "shader");
        return false;
#endif
    default:
        SDL_SetError("Unknown present filter %d", (int)mode);
        return false;
    }
    if (s_texture && SDL_SetTextureScaleMode(s_texture, resolved) != 0)
        return false;
    const bool changed = resolved != s_scaleMode || glFilter != s_glFilter;
    s_scaleMode = resolved;
    s_glFilter = glFilter;
    if (changed && s_renderer && s_texture)
        PresentTexture();
    return true;
}
void SDL2_SetRelativeMouse(bool enabled) {
    if (s_window && SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE) != 0)
        Platform_Log(PLATFORM_LOG_WARN, "Unable to change SDL2 relative mouse mode: %s", SDL_GetError());
}
void SDL2_SetMouseGrab(bool enabled) {
    if (s_window)
        SDL_SetWindowGrab(s_window, enabled ? SDL_TRUE : SDL_FALSE);
}
void SDL2_HandleWindowEvent(const SDL_Event *event) {
    if (!s_window || event->type != SDL_WINDOWEVENT)
        return;
    if (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED || event->window.event == SDL_WINDOWEVENT_ENTER)
        SDL_ShowCursor(SDL_DISABLE);
    else if (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED && s_renderer && s_texture)
        PresentTexture();
}
bool SDL2_Present(const PlatformFrame *frame) {
    if (!frame || !s_texture || !s_renderer || frame->width != s_width || frame->height != s_height) {
        SDL_SetError("Indexed frame does not match the presentation surface");
        return Error("SDL2 present");
    }
    void *pixels = NULL;
    int pitch = 0;
    if (SDL_LockTexture(s_texture, NULL, &pixels, &pitch) != 0)
        return Error("Unable to lock SDL2 presentation texture");
    bool converted = Platform_ConvertIndexedFrame(frame, (uint32_t *)pixels, (uint32_t)pitch);
    if (converted)
        ApplyBackdrop(frame, (uint32_t *)pixels, (uint32_t)pitch);
    else
        s_backdropOn = false;
    if (converted && frame->overlay)
        frame->overlay((uint32_t *)pixels, (uint32_t)pitch, frame->width, frame->height, frame->palette_rgb);
    SDL_UnlockTexture(s_texture);
    return converted && PresentTexture();
}

/* dst_* is in framebuffer pixels; map it into the same letterboxed viewport
   the indexed frame is drawn to. fill_display covers the whole output
   instead, cropping the frame to the output's aspect. */
bool SDL2_PresentVideo(const PlatformVideoFrame *frame) {
    if (!frame || !frame->y || !frame->u || !frame->v || !frame->width || !frame->height ||
        !s_renderer || !s_width || !s_height) {
        SDL_SetError("Invalid movie frame");
        return Error("SDL2 present");
    }
    if (!s_videoTexture || s_videoWidth != frame->width || s_videoHeight != frame->height) {
        SDL_DestroyTexture(s_videoTexture);
        /* MPEG-1 decodes BT.601; SDL2 would pick BT.709 for a frame this tall. */
        SDL_SetYUVConversionMode(SDL_YUV_CONVERSION_BT601);
        s_videoTexture = SDL_CreateTexture(s_renderer, SDL_PIXELFORMAT_IYUV,
                                           SDL_TEXTUREACCESS_STREAMING, (int)frame->width,
                                           (int)frame->height);
        if (!s_videoTexture) {
            s_videoWidth = s_videoHeight = 0;
            return Error("Unable to create SDL2 movie texture");
        }
        SDL_SetTextureScaleMode(s_videoTexture, SDL_ScaleModeLinear);
        s_videoWidth = frame->width;
        s_videoHeight = frame->height;
    }
    if (SDL_UpdateYUVTexture(s_videoTexture, NULL, frame->y, (int)frame->y_pitch, frame->u,
                             (int)frame->uv_pitch, frame->v, (int)frame->uv_pitch) != 0)
        return Error("Unable to upload SDL2 movie frame");
    SDL_SetTextureColorMod(s_videoTexture, frame->brightness, frame->brightness, frame->brightness);
    int width, height;
    if (SDL_GetRendererOutputSize(s_renderer, &width, &height) != 0)
        return Error("Unable to query SDL2 render dimensions");
    SDL_Rect src = {0, 0, (int)frame->width, (int)frame->height};
    SDL_Rect dst;
    if (frame->fill_display) {
        dst.x = dst.y = 0;
        dst.w = width;
        dst.h = height;
        if ((double)frame->width * height > (double)frame->height * width) {
            src.w = (int)((double)frame->height * width / height);
            src.x = ((int)frame->width - src.w) / 2;
        } else {
            src.h = (int)((double)frame->width * height / width);
            src.y = ((int)frame->height - src.h) / 2;
        }
    } else {
        const SDL_Rect area = Viewport(width, height);
        dst.x = area.x + (int)((double)frame->dst_x * area.w / s_width);
        dst.y = area.y + (int)((double)frame->dst_y * area.h / s_height);
        dst.w = (int)((double)frame->dst_w * area.w / s_width);
        dst.h = (int)((double)frame->dst_h * area.h / s_height);
    }
    if (SDL_SetRenderDrawColor(s_renderer, 0, 0, 0, 255) != 0 || SDL_RenderClear(s_renderer) != 0 ||
        SDL_RenderCopy(s_renderer, s_videoTexture, &src, &dst) != 0)
        return Error("Unable to present SDL2 movie frame");
    SDL_RenderPresent(s_renderer);
    return true;
}
