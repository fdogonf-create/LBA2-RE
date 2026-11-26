#include "UNLOCKS.H"

#include "DIRECTORIES.H"

#include <SYSTEM/LIMITS.H>
#include <SYSTEM/LOG.H>

#include <stdio.h>
#include <stdlib.h>

/* Beside the saves, so it follows the profile the saves belong to. */
#define UNLOCKS_FILENAME "unlocks.txt"

static int s_loaded = 0;

void Unlocks_Load(void) {
    if (s_loaded)
        return;
    s_loaded = 1;

    char path[ADELINE_MAX_PATH];
    GetSavePath(path, ADELINE_MAX_PATH, UNLOCKS_FILENAME);
    FILE *f = fopen(path, "rb");
    if (!f)
        return;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size > 0 && size < 256 * 1024) {
        char *text = (char *)malloc((size_t)size + 1);
        if (text) {
            size_t got = fread(text, 1, (size_t)size, f);
            text[got] = '\0';
            Unlocks_Parse(text);
            free(text);
        }
    }
    fclose(f);
}

static void Unlocks_Save(void) {
    size_t need = Unlocks_Format(NULL, 0);
    char *text = (char *)malloc(need + 1);
    if (!text)
        return;
    Unlocks_Format(text, need + 1);

    char path[ADELINE_MAX_PATH];
    GetSavePath(path, ADELINE_MAX_PATH, UNLOCKS_FILENAME);
    FILE *f = fopen(path, "wb");
    if (!f) {
        Log_Warn("Could not write %s", path);
        free(text);
        return;
    }
    fwrite(text, 1, need, f);
    fclose(f);
    free(text);
}

void Unlocks_Mark(const char *category, const char *name) {
    Unlocks_Load();
    if (Unlocks_Add(category, name))
        Unlocks_Save();
}
