/* Shader present filters for the SDL2 backend: the indexed frame, already
 * converted to the ARGB streaming texture, is drawn to the letterboxed viewport
 * by a small GLES2 program of ours instead of SDL_RenderCopy. Only for SDL's
 * "opengles2" renderer, which is what the Switch build uses; elsewhere these
 * filters report themselves unavailable and the menu steps past them.
 *
 * SDL keeps its own GL state cache, so the draw flushes SDL's queue first and
 * puts back every piece of state it touches before handing the context back.
 * GL entry points come from SDL_GL_GetProcAddress, so no GL headers or link
 * libraries are needed. */
#include "PRIVATE.H"

#include "GLFILTER.H"

#if LBA2_SDL2_GL_FILTERS

#include <string.h>

#if defined(_WIN32)
#define LBA2_GLAPI __stdcall
#else
#define LBA2_GLAPI
#endif

namespace {
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef float GLfloat;
typedef unsigned char GLboolean;
typedef char GLchar;

const GLenum GL_TRIANGLE_STRIP = 0x0005;
const GLenum GL_FLOAT = 0x1406;
const GLenum GL_BLEND = 0x0BE2;
const GLenum GL_SCISSOR_TEST = 0x0C11;
const GLenum GL_CULL_FACE = 0x0B44;
const GLenum GL_DEPTH_TEST = 0x0B71;
const GLenum GL_VIEWPORT = 0x0BA2;
const GLenum GL_TEXTURE0 = 0x84C0;
const GLenum GL_ACTIVE_TEXTURE = 0x84E0;
const GLenum GL_ARRAY_BUFFER = 0x8892;
const GLenum GL_ARRAY_BUFFER_BINDING = 0x8894;
const GLenum GL_CURRENT_PROGRAM = 0x8B8D;
const GLenum GL_VERTEX_ATTRIB_ARRAY_ENABLED = 0x8622;
const GLenum GL_FRAGMENT_SHADER = 0x8B30;
const GLenum GL_VERTEX_SHADER = 0x8B31;
const GLenum GL_COMPILE_STATUS = 0x8B81;
const GLenum GL_LINK_STATUS = 0x8B82;

struct GlApi {
    GLuint(LBA2_GLAPI *CreateShader)(GLenum);
    void(LBA2_GLAPI *ShaderSource)(GLuint, GLsizei, const GLchar *const *, const GLint *);
    void(LBA2_GLAPI *CompileShader)(GLuint);
    void(LBA2_GLAPI *GetShaderiv)(GLuint, GLenum, GLint *);
    void(LBA2_GLAPI *GetShaderInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
    void(LBA2_GLAPI *DeleteShader)(GLuint);
    GLuint(LBA2_GLAPI *CreateProgram)(void);
    void(LBA2_GLAPI *AttachShader)(GLuint, GLuint);
    void(LBA2_GLAPI *BindAttribLocation)(GLuint, GLuint, const GLchar *);
    void(LBA2_GLAPI *LinkProgram)(GLuint);
    void(LBA2_GLAPI *GetProgramiv)(GLuint, GLenum, GLint *);
    void(LBA2_GLAPI *GetProgramInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
    void(LBA2_GLAPI *UseProgram)(GLuint);
    GLint(LBA2_GLAPI *GetUniformLocation)(GLuint, const GLchar *);
    void(LBA2_GLAPI *Uniform1i)(GLint, GLint);
    void(LBA2_GLAPI *Uniform2f)(GLint, GLfloat, GLfloat);
    void(LBA2_GLAPI *GetIntegerv)(GLenum, GLint *);
    GLboolean(LBA2_GLAPI *IsEnabled)(GLenum);
    void(LBA2_GLAPI *Enable)(GLenum);
    void(LBA2_GLAPI *Disable)(GLenum);
    void(LBA2_GLAPI *Viewport)(GLint, GLint, GLsizei, GLsizei);
    void(LBA2_GLAPI *BindBuffer)(GLenum, GLuint);
    void(LBA2_GLAPI *VertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
    void(LBA2_GLAPI *EnableVertexAttribArray)(GLuint);
    void(LBA2_GLAPI *DisableVertexAttribArray)(GLuint);
    void(LBA2_GLAPI *GetVertexAttribiv)(GLuint, GLenum, GLint *);
    void(LBA2_GLAPI *DrawArrays)(GLenum, GLint, GLsizei);
    void(LBA2_GLAPI *ActiveTexture)(GLenum);
};

/* SDL's GLES2 renderer binds its attributes 0..4; ours are 0 and 1. */
const GLuint ATTRIB_SAVED = 5;

GlApi s_gl;
SDL_Renderer *s_glRenderer = NULL; /* the renderer s_gl and the programs belong to */
bool s_glReady = false;
GLuint s_programs[LBA2_GL_FILTER_COUNT];
bool s_programFailed[LBA2_GL_FILTER_COUNT];

const char *const kVertex = "attribute vec2 aPos;\n"
                            "attribute vec2 aUV;\n"
                            "varying vec2 vUV;\n"
                            "void main() {\n"
                            "    vUV = aUV;\n"
                            "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
                            "}\n";

/* SDL's GLES2 renderer stores an ARGB8888 texture's bytes as RGBA, so a texel
 * reads back with red and blue swapped: T() swaps them back. */
const char *const kFragment =
    "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
    "precision highp float;\n"
    "#else\n"
    "precision mediump float;\n"
    "#endif\n"
    "uniform sampler2D uTex;\n"
    "uniform vec2 uSrc;\n" /* frame size, texels */
    "uniform vec2 uOut;\n" /* viewport size, pixels */
    "varying vec2 vUV;\n"
    "vec3 T(vec2 uv) { return texture2D(uTex, uv).bgr; }\n"
    "float L(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }\n"
    /* Pixel art: each texel's inside stays flat, only the seam between two
       texels is blended, one output pixel wide. */
    "vec3 Sharp(vec2 uv) {\n"
    "    vec2 scale = max(uOut / uSrc, vec2(1.0));\n"
    "    vec2 texel = uv * uSrc;\n"
    "    vec2 base = floor(texel);\n"
    "    vec2 d = fract(texel) - 0.5;\n"
    "    vec2 range = 0.5 - 0.5 / scale;\n"
    "    vec2 f = (d - clamp(d, -range, range)) * scale + 0.5;\n"
    "    return T((base + f) / uSrc);\n"
    "}\n"
    /* Smoothing: FXAA run in the frame's own texels, so it finds the stairs
       of the low-resolution picture; away from an edge the pixel-art sample. */
    "vec3 Smooth(vec2 uv) {\n"
    "    vec2 rcp = 1.0 / uSrc;\n"
    "    vec3 m = Sharp(uv);\n"
    "    float lNW = L(T(uv + vec2(-1.0, -1.0) * rcp));\n"
    "    float lNE = L(T(uv + vec2(1.0, -1.0) * rcp));\n"
    "    float lSW = L(T(uv + vec2(-1.0, 1.0) * rcp));\n"
    "    float lSE = L(T(uv + vec2(1.0, 1.0) * rcp));\n"
    "    float lM = L(m);\n"
    "    float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));\n"
    "    float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));\n"
    "    if (lMax - lMin < max(0.04, lMax * 0.12)) return m;\n"
    "    vec2 dir = vec2(-((lNW + lNE) - (lSW + lSE)), (lNW + lSW) - (lNE + lSE));\n"
    "    float reduce = max((lNW + lNE + lSW + lSE) * 0.03125, 0.0078125);\n"
    "    float rcpMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + reduce);\n"
    "    dir = clamp(dir * rcpMin, vec2(-6.0), vec2(6.0)) * rcp;\n"
    "    vec3 a = 0.5 * (T(uv + dir * (1.0 / 3.0 - 0.5)) + T(uv + dir * (2.0 / 3.0 - 0.5)));\n"
    "    vec3 b = a * 0.5 + 0.25 * (T(uv - dir * 0.5) + T(uv + dir * 0.5));\n"
    "    float lB = L(b);\n"
    "    return (lB < lMin || lB > lMax) ? a : b;\n"
    "}\n"
    /* Old TV: a soft dark gap between the frame's lines, a faint RGB stripe
       mask, a light vignette; brightened back up to make up for the dark. The
       gaps are weaker where a line is under two output pixels tall, where they
       would only beat against the pixel grid. */
    "vec3 Crt(vec3 c, vec2 uv) {\n"
    "    float perLine = uOut.y / uSrc.y;\n"
    "    float strength = clamp((perLine - 1.0) * 0.3, 0.12, 0.38);\n"
    "    float y = fract(uv.y * uSrc.y) - 0.5;\n"
    "    float scan = 1.0 - strength + strength * exp(-y * y * 10.0);\n"
    "    float m = mod(floor(gl_FragCoord.x), 3.0);\n"
    "    vec3 mask = vec3(0.9);\n"
    "    if (m < 0.5) mask.r = 1.06; else if (m < 1.5) mask.g = 1.06; else mask.b = 1.06;\n"
    "    vec2 v = uv - 0.5;\n"
    "    float vignette = 1.0 - dot(v, v) * 0.45;\n"
    "    return min(c * scan * mask * vignette * (1.0 + strength * 0.75), vec3(1.0));\n"
    "}\n"
    "void main() {\n"
    "#if MODE == 0\n"
    "    vec3 c = Sharp(vUV);\n"
    "#elif MODE == 1\n"
    "    vec3 c = Smooth(vUV);\n"
    "#elif MODE == 2\n"
    "    vec3 c = Crt(T(vec2(vUV.x, (floor(vUV.y * uSrc.y) + 0.5) / uSrc.y)), vUV);\n"
    "#else\n"
    "    vec3 c = Crt(Smooth(vUV), vUV);\n"
    "#endif\n"
    "    gl_FragColor = vec4(c, 1.0);\n"
    "}\n";

template <typename F>
bool Load(F *slot, const char *name) {
    void *proc = SDL_GL_GetProcAddress(name);
    memcpy(slot, &proc, sizeof(proc));
    return proc != NULL;
}

bool LoadApi() {
    bool ok = true;
    ok = Load(&s_gl.CreateShader, "glCreateShader") && ok;
    ok = Load(&s_gl.ShaderSource, "glShaderSource") && ok;
    ok = Load(&s_gl.CompileShader, "glCompileShader") && ok;
    ok = Load(&s_gl.GetShaderiv, "glGetShaderiv") && ok;
    ok = Load(&s_gl.GetShaderInfoLog, "glGetShaderInfoLog") && ok;
    ok = Load(&s_gl.DeleteShader, "glDeleteShader") && ok;
    ok = Load(&s_gl.CreateProgram, "glCreateProgram") && ok;
    ok = Load(&s_gl.AttachShader, "glAttachShader") && ok;
    ok = Load(&s_gl.BindAttribLocation, "glBindAttribLocation") && ok;
    ok = Load(&s_gl.LinkProgram, "glLinkProgram") && ok;
    ok = Load(&s_gl.GetProgramiv, "glGetProgramiv") && ok;
    ok = Load(&s_gl.GetProgramInfoLog, "glGetProgramInfoLog") && ok;
    ok = Load(&s_gl.UseProgram, "glUseProgram") && ok;
    ok = Load(&s_gl.GetUniformLocation, "glGetUniformLocation") && ok;
    ok = Load(&s_gl.Uniform1i, "glUniform1i") && ok;
    ok = Load(&s_gl.Uniform2f, "glUniform2f") && ok;
    ok = Load(&s_gl.GetIntegerv, "glGetIntegerv") && ok;
    ok = Load(&s_gl.IsEnabled, "glIsEnabled") && ok;
    ok = Load(&s_gl.Enable, "glEnable") && ok;
    ok = Load(&s_gl.Disable, "glDisable") && ok;
    ok = Load(&s_gl.Viewport, "glViewport") && ok;
    ok = Load(&s_gl.BindBuffer, "glBindBuffer") && ok;
    ok = Load(&s_gl.VertexAttribPointer, "glVertexAttribPointer") && ok;
    ok = Load(&s_gl.EnableVertexAttribArray, "glEnableVertexAttribArray") && ok;
    ok = Load(&s_gl.DisableVertexAttribArray, "glDisableVertexAttribArray") && ok;
    ok = Load(&s_gl.GetVertexAttribiv, "glGetVertexAttribiv") && ok;
    ok = Load(&s_gl.DrawArrays, "glDrawArrays") && ok;
    ok = Load(&s_gl.ActiveTexture, "glActiveTexture") && ok;
    return ok;
}

GLuint Compile(GLenum type, const char *prefix, const char *body) {
    const GLchar *sources[2] = {prefix, body};
    GLuint shader = s_gl.CreateShader(type);
    GLint status = 0;
    if (!shader)
        return 0;
    s_gl.ShaderSource(shader, 2, sources, NULL);
    s_gl.CompileShader(shader);
    s_gl.GetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        log[0] = '\0';
        s_gl.GetShaderInfoLog(shader, (GLsizei)sizeof(log), NULL, log);
        Platform_Log(PLATFORM_LOG_WARN, "Present filter shader did not compile: %s", log);
        s_gl.DeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint Program(int filter) {
    if (filter < 0 || filter >= LBA2_GL_FILTER_COUNT || s_programFailed[filter])
        return 0;
    if (s_programs[filter])
        return s_programs[filter];
    char prefix[32];
    SDL_snprintf(prefix, sizeof(prefix), "#define MODE %d\n", filter);
    GLuint vs = Compile(GL_VERTEX_SHADER, "", kVertex);
    GLuint fs = vs ? Compile(GL_FRAGMENT_SHADER, prefix, kFragment) : 0;
    GLuint program = 0;
    if (vs && fs) {
        GLint status = 0;
        program = s_gl.CreateProgram();
        s_gl.AttachShader(program, vs);
        s_gl.AttachShader(program, fs);
        s_gl.BindAttribLocation(program, 0, "aPos");
        s_gl.BindAttribLocation(program, 1, "aUV");
        s_gl.LinkProgram(program);
        s_gl.GetProgramiv(program, GL_LINK_STATUS, &status);
        if (!status) {
            char log[512];
            log[0] = '\0';
            s_gl.GetProgramInfoLog(program, (GLsizei)sizeof(log), NULL, log);
            Platform_Log(PLATFORM_LOG_WARN, "Present filter shader did not link: %s", log);
            program = 0;
        }
    }
    if (vs)
        s_gl.DeleteShader(vs);
    if (fs)
        s_gl.DeleteShader(fs);
    if (!program)
        s_programFailed[filter] = true;
    s_programs[filter] = program;
    return program;
}
} // namespace

bool SDL2_GLFilterAvailable(SDL_Renderer *renderer) {
    if (!renderer)
        return false;
    if (renderer != s_glRenderer) {
        SDL_RendererInfo info;
        s_glRenderer = renderer;
        s_glReady = SDL_GetRendererInfo(renderer, &info) == 0 && info.name &&
                    SDL_strcmp(info.name, "opengles2") == 0 && LoadApi();
        memset(s_programs, 0, sizeof(s_programs));
        memset(s_programFailed, 0, sizeof(s_programFailed));
    }
    return s_glReady;
}

void SDL2_GLFilterForget(SDL_Renderer *renderer) {
    if (renderer == s_glRenderer) {
        s_glRenderer = NULL;
        s_glReady = false;
    }
}

bool SDL2_GLFilterDraw(SDL_Renderer *renderer, SDL_Texture *texture, int filter, uint32_t srcW, uint32_t srcH,
                       const SDL_Rect *area, int outH) {
    if (!SDL2_GLFilterAvailable(renderer) || SDL_RenderFlush(renderer) != 0)
        return false;
    /* Compiling needs the context current, which binding makes it. */
    GLint activeTexture = GL_TEXTURE0;
    s_gl.GetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
    s_gl.ActiveTexture(GL_TEXTURE0);
    float texW = 1.0f, texH = 1.0f;
    if (SDL_GL_BindTexture(texture, &texW, &texH) != 0) {
        s_gl.ActiveTexture((GLenum)activeTexture);
        return false;
    }
    GLuint program = Program(filter);
    if (!program) {
        SDL_GL_UnbindTexture(texture);
        s_gl.ActiveTexture((GLenum)activeTexture);
        return false;
    }

    GLint savedProgram = 0, savedBuffer = 0, savedViewport[4] = {0, 0, 0, 0};
    GLint savedAttrib[ATTRIB_SAVED];
    const GLenum caps[4] = {GL_BLEND, GL_SCISSOR_TEST, GL_CULL_FACE, GL_DEPTH_TEST};
    GLboolean savedCaps[4];
    GLuint i;
    s_gl.GetIntegerv(GL_CURRENT_PROGRAM, &savedProgram);
    s_gl.GetIntegerv(GL_ARRAY_BUFFER_BINDING, &savedBuffer);
    s_gl.GetIntegerv(GL_VIEWPORT, savedViewport);
    for (i = 0; i < ATTRIB_SAVED; i++) {
        savedAttrib[i] = 0;
        s_gl.GetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &savedAttrib[i]);
    }
    for (i = 0; i < 4; i++) {
        savedCaps[i] = s_gl.IsEnabled(caps[i]);
        s_gl.Disable(caps[i]);
    }

    const GLfloat positions[8] = {-1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f};
    const GLfloat uvs[8] = {0.0f, 0.0f, 0.0f, texH, texW, 0.0f, texW, texH};
    s_gl.UseProgram(program);
    s_gl.Uniform1i(s_gl.GetUniformLocation(program, "uTex"), 0);
    s_gl.Uniform2f(s_gl.GetUniformLocation(program, "uSrc"), (GLfloat)srcW, (GLfloat)srcH);
    s_gl.Uniform2f(s_gl.GetUniformLocation(program, "uOut"), (GLfloat)area->w, (GLfloat)area->h);
    s_gl.Viewport(area->x, outH - area->y - area->h, area->w, area->h);
    s_gl.BindBuffer(GL_ARRAY_BUFFER, 0);
    for (i = 0; i < ATTRIB_SAVED; i++)
        s_gl.DisableVertexAttribArray(i);
    s_gl.EnableVertexAttribArray(0);
    s_gl.EnableVertexAttribArray(1);
    s_gl.VertexAttribPointer(0, 2, GL_FLOAT, 0, 0, positions);
    s_gl.VertexAttribPointer(1, 2, GL_FLOAT, 0, 0, uvs);
    s_gl.DrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    for (i = 0; i < ATTRIB_SAVED; i++) {
        if (savedAttrib[i])
            s_gl.EnableVertexAttribArray(i);
        else
            s_gl.DisableVertexAttribArray(i);
    }
    for (i = 0; i < 4; i++) {
        if (savedCaps[i])
            s_gl.Enable(caps[i]);
    }
    s_gl.BindBuffer(GL_ARRAY_BUFFER, (GLuint)savedBuffer);
    s_gl.Viewport(savedViewport[0], savedViewport[1], savedViewport[2], savedViewport[3]);
    s_gl.UseProgram((GLuint)savedProgram);
    SDL_GL_UnbindTexture(texture);
    s_gl.ActiveTexture((GLenum)activeTexture);
    return true;
}

#endif
