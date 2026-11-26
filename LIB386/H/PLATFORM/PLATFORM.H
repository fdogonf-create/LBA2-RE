#pragma once

#include <PLATFORM/INPUT.H>

#include <stddef.h>
#include <stdint.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PlatformEventType {
    PLATFORM_EVENT_OTHER,
    PLATFORM_EVENT_QUIT,
    PLATFORM_EVENT_WINDOW_FOCUS_GAINED,
    PLATFORM_EVENT_WINDOW_FOCUS_LOST,
    PLATFORM_EVENT_WINDOW_MOUSE_ENTER,
    PLATFORM_EVENT_WINDOW_RESIZED,
    PLATFORM_EVENT_KEY_DOWN,
    PLATFORM_EVENT_KEY_UP,
    PLATFORM_EVENT_TEXT_INPUT,
    PLATFORM_EVENT_MOUSE_MOTION,
    PLATFORM_EVENT_MOUSE_BUTTON_DOWN,
    PLATFORM_EVENT_MOUSE_BUTTON_UP,
    PLATFORM_EVENT_MOUSE_WHEEL,
    PLATFORM_EVENT_GAMEPAD_ADDED,
    PLATFORM_EVENT_GAMEPAD_REMOVED,
    PLATFORM_EVENT_GAMEPAD_BUTTON_DOWN,
    PLATFORM_EVENT_GAMEPAD_AXIS_MOTION,
    PLATFORM_EVENT_FINGER_DOWN,
    PLATFORM_EVENT_FINGER_UP,
    PLATFORM_EVENT_FINGER_MOTION
} PlatformEventType;

/* Text is borrowed until the next event poll. Coordinates are window pixels;
   finger coordinates are normalised to the whole display, including bars.
   A finger also arrives as mouse motion and buttons, so a tap still points and
   clicks; `touch` marks those, so the pointer can act without being drawn. */
typedef struct PlatformEvent {
    PlatformEventType type;
    union {
        struct {
            int32_t scancode;
            uint16_t mod;
            bool repeat;
        } key;
        struct {
            const char *text;
        } text;
        struct {
            float x, y, xrel, yrel;
            bool touch;
        } motion;
        struct {
            float x, y;
            uint8_t button;
            bool touch;
        } button;
        struct {
            float y;
        } wheel;
        struct {
            uint32_t which;
        } gdevice;
        struct {
            uint32_t which;
            PlatformGamepadAxis axis;
            int16_t value;
        } gaxis;
        struct {
            uint64_t fingerID;
            float x, y;
            bool indirect; /* trackpad/rear panel, not the display */
        } tfinger;
    };
} PlatformEvent;

typedef void (*PlatformOverlayFn)(uint32_t *pixels, uint32_t pitch,
                                  uint32_t width, uint32_t height,
                                  const uint8_t *palette_rgb);

/* Indexed pixels and the 256 RGB triples are borrowed for this call only.
   The optional overlay operates on 0xAARRGGBB pixels, with a byte pitch.
   Backends without an overlay may present indexed/RGB565 pixels directly. */
/* A full-colour picture shown in place of part of the indexed frame: inside
   the rectangle (x, y, w, h, in frame pixels), every frame pixel whose index
   still equals `key` there is the picture, scaled to the rectangle at the
   output's own resolution; any other pixel (something drawn over it) stays
   the frame's. The picture is dimmed by how far the frame's palette has faded
   from `key_palette`. All of it is borrowed until it is changed or cleared. A
   backend that cannot show it presents the frame alone. */
typedef struct PlatformBackdrop {
    const uint32_t *argb; /* 0xAARRGGBB, `width` x `height`, rows packed */
    uint32_t width, height;
    int32_t x, y, w, h;
    const uint8_t *key; /* w x h indices, rows packed */
    const uint8_t *key_palette;
    uint32_t serial; /* changes whenever argb's contents do */
} PlatformBackdrop;

typedef struct PlatformFrame {
    const uint8_t *pixels;
    uint32_t width, height, pitch;
    const uint8_t *palette_rgb;
    PlatformOverlayFn overlay;
    const PlatformBackdrop *backdrop; /* optional, NULL for none */
} PlatformFrame;

/* One frame of a full-colour movie: planar YUV 4:2:0 (I420, BT.601 limited
   range, as MPEG-1 decodes), converted on the GPU. dst_* is a rectangle of the
   framebuffer in framebuffer pixels; fill_display instead covers the whole
   display, cropping the frame to its aspect. brightness scales the image
   (255 = as decoded) for fades. Everything outside the image is black. The
   planes are borrowed for this call only. */
typedef struct PlatformVideoFrame {
    const uint8_t *y, *u, *v;
    uint32_t y_pitch, uv_pitch;
    uint32_t width, height;
    int32_t dst_x, dst_y, dst_w, dst_h;
    bool fill_display;
    uint8_t brightness;
} PlatformVideoFrame;

typedef struct PlatformGamepad PlatformGamepad;
typedef struct PlatformFolderDialog PlatformFolderDialog;

typedef enum PlatformDialogResult {
    PLATFORM_DIALOG_PENDING,
    PLATFORM_DIALOG_SELECTED,
    PLATFORM_DIALOG_CANCELLED,
    PLATFORM_DIALOG_ERROR
} PlatformDialogResult;

typedef enum PlatformMessageKind {
    PLATFORM_MESSAGE_INFORMATION,
    PLATFORM_MESSAGE_WARNING,
    PLATFORM_MESSAGE_ERROR
} PlatformMessageKind;

typedef enum PlatformLogLevel {
    PLATFORM_LOG_DEBUG,
    PLATFORM_LOG_INFO,
    PLATFORM_LOG_WARN,
    PLATFORM_LOG_ERROR
} PlatformLogLevel;

typedef void (*PlatformLogFn)(void *userdata, PlatformLogLevel level, const char *message);

/* How a frame is filtered when it is stretched to the window. The numbers are
   what a config file stores, so they do not move. */
typedef enum PlatformPresentFilter {
    PLATFORM_FILTER_NEAREST = 0,
    PLATFORM_FILTER_LINEAR = 1,
    PLATFORM_FILTER_PIXELART = 2, /* crisp texel interiors, interpolated seams */
    PLATFORM_FILTER_SMOOTH = 3,   /* pixel art, with the stairs on edges smoothed (FXAA) */
    PLATFORM_FILTER_CRT = 4,      /* an old TV: scanlines, stripe mask, vignette */
    PLATFORM_FILTER_SMOOTH_CRT = 5
} PlatformPresentFilter;

typedef struct PlatformCapabilities {
    bool windowed;
    bool transient_window_loss;
    bool touch_overlay;
    bool touch_recenter; /* front-screen presses recenter during gameplay */
} PlatformCapabilities;

typedef struct PlatformSystem {
    bool (*init)(void);
    void (*shutdown)(void);
    bool (*set_headless)(void); /* before init; retain a present-capable offscreen target */
    const char *(*get_error)(void);
    void (*clear_error)(void);
    uint64_t (*ticks_ms)(void); /* wall clock; the engine owns its fixed-dt overlay */
    void (*delay_ms)(uint32_t ms);
    uint64_t (*performance_counter)(void);
    uint64_t (*performance_frequency)(void);
    uint64_t (*thread_id)(void);
    int32_t (*cpu_count)(void);
    int32_t (*ram_mb)(void);
    const char *(*platform_name)(void);
    void (*set_log_output)(PlatformLogFn callback, void *userdata);
} PlatformSystem;

typedef struct PlatformEvents {
    bool (*init)(void);
    void (*shutdown)(void);
    void (*pump)(void);
    bool (*poll)(PlatformEvent *event);
} PlatformEvents;

typedef struct PlatformWindow {
    bool (*init)(const char *title);
    void (*shutdown)(void);
    bool (*initialized)(void);
    bool (*create)(uint32_t width, uint32_t height, bool fullscreen);
    void (*destroy)(void);
    bool (*resize)(uint32_t width, uint32_t height, bool keep_window_size);
    bool (*size)(int32_t *width, int32_t *height);
    bool (*display_size)(uint32_t *width, uint32_t *height);
    bool (*primary_display_size)(uint32_t *width, uint32_t *height);
    void (*surface_coords)(int32_t x, int32_t y, int32_t *sx, int32_t *sy);
    void (*set_fullscreen)(bool enabled);
    bool (*get_fullscreen)(void); /* requested state, including failed-change rollback */
    bool (*is_fullscreen)(void);  /* actual host state */
    void (*set_vsync)(bool enabled);
    /* Kept across surface recreation. A mode the backend cannot do is refused
       with an error and leaves the current one in place. */
    bool (*set_present_filter)(PlatformPresentFilter mode);
    void (*set_relative_mouse)(bool enabled);
    /* Keep the cursor inside the window. The host lifts it while the window
       is unfocused and puts it back on focus, as it does relative mode. */
    void (*set_mouse_grab)(bool enabled);
    bool (*present)(const PlatformFrame *frame);
    /* False on a backend that can only show indexed frames; the caller then
       shows its indexed fallback. */
    bool (*present_video)(const PlatformVideoFrame *frame);
} PlatformWindow;

typedef struct PlatformInput {
    /* The sampled state has PLATFORM_SCANCODE_COUNT entries, 0 or 1 per key. */
    const uint8_t *(*keyboard_state)(void);
    uint32_t (*keycode)(int32_t scancode, uint16_t modifiers);
    const char *(*scancode_name)(int32_t scancode);
    uint16_t (*modifiers)(void);
    void (*text_input)(bool enabled);
    void (*cursor_visible)(bool visible);
    bool (*init_gamepads)(void);
    void (*shutdown_gamepads)(void);
    uint32_t *(*gamepads)(int32_t *count); /* owned; free_memory */
    PlatformGamepad *(*open_gamepad)(uint32_t id);
    void (*close_gamepad)(PlatformGamepad *pad);
    const char *(*gamepad_name)(PlatformGamepad *pad); /* borrowed while pad is open */
    bool (*gamepad_button)(PlatformGamepad *pad, PlatformGamepadButton button);
    int16_t (*gamepad_axis)(PlatformGamepad *pad, PlatformGamepadAxis axis);
} PlatformInput;

typedef struct PlatformHost {
    const char *(*base_path)(void);                       /* borrowed for backend lifetime */
    char *(*current_directory)(void);                     /* owned; free_memory */
    char *(*pref_path)(const char *org, const char *app); /* owned; free_memory */
    const char *(*home_path)(void);                       /* borrowed */
    bool (*create_directory)(const char *path);
    bool (*remove_path)(const char *path); /* absent path is already removed; directories must be empty */
    bool (*message_box)(PlatformMessageKind kind, const char *title, const char *message);
    /* Results are delivered on the polling thread. Path is borrowed until end_dialog;
       an error must be available through system.get_error on that same thread. */
    PlatformFolderDialog *(*begin_folder_dialog)(const char *initial_path);
    PlatformDialogResult (*poll_folder_dialog)(PlatformFolderDialog *dialog, const char **path);
    void (*end_folder_dialog)(PlatformFolderDialog *dialog); /* completed requests only */
    bool (*set_clipboard)(const char *text);
    char *(*get_clipboard)(void); /* owned; free_memory */
    void (*free_memory)(void *ptr);
} PlatformHost;

/* Exactly one implementation is selected at link time. Every callback is
   supplied: unsupported host services report an error, never a success stub. */
typedef struct PlatformInterface {
    const char *name;
    PlatformCapabilities capabilities;
    PlatformSystem system;
    PlatformEvents events;
    PlatformWindow window;
    PlatformInput input;
    PlatformHost host;
} PlatformInterface;

extern const PlatformInterface g_platform;

/* Backend diagnostics have no dependency on the game's logger. */
void Platform_SetLogOutput(PlatformLogFn callback, void *userdata);
void Platform_Log(PlatformLogLevel level, const char *fmt, ...);

/* The app is about to be suspended, after which the system may end it without
   another word (UWP on the Xbox). The game registers what must be kept; a
   backend that suspends calls Platform_NotifySuspend from its suspend handler,
   on the game's thread, before letting the app be frozen. */
typedef void (*PlatformSuspendFn)(void);
void Platform_SetSuspendHandler(PlatformSuspendFn handler);
void Platform_NotifySuspend(void);

/* Shared software-present conversion; pitches are bytes. Padding is untouched. */
bool Platform_ConvertIndexedFrame(const PlatformFrame *frame, uint32_t *pixels, uint32_t pitch);

#ifdef __cplusplus
}
#endif
