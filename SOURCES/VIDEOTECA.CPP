#include <SYSTEM/ADELINE_TYPES.H>
#include <SYSTEM/ADELINE.H>
#include <SYSTEM/LIMITS.H>

#include "DIRECTORIES.H"
#include "MESSAGE.H"
#include "PLAYACF.H"
#include "UNLOCKS.H"
#include "VIDEOTECA.H"

#include <SYSTEM/STRING.H>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ListAcf numbers the cinematics; that number is the bit each one sets in the
   save's FLAG_ACF words, which hold three words of 16. */
#define VIDEOTECA_MAX 48
#define VIDEOTECA_NAME 16

static char s_names[VIDEOTECA_MAX][VIDEOTECA_NAME];
static int s_total = -1;

static void LoadNames(void) {
    if (s_total >= 0)
        return;
    s_total = 0;
    if (!ListAcf)
        return;
    char buf[128];
    ResetAcfList();
    while (s_total < VIDEOTECA_MAX && GetAcfName(buf)) {
        size_t n = 0;
        while (buf[n] && buf[n] != '.' && n < VIDEOTECA_NAME - 1) {
            char c = buf[n];
            s_names[s_total][n] = (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
            n++;
        }
        s_names[s_total][n] = '\0';
        s_total++;
    }
}

static int IsKnown(const char *name) {
    LoadNames();
    for (int i = 0; i < s_total; i++) {
        if (!strcmp(s_names[i], name))
            return 1;
    }
    return 0;
}

void Videoteca_NoteSeenFromSave(void) {
    LoadNames();
    for (int i = 0; i < s_total && i < ACF_FLAG_WORDS * 16; i++) {
        if (AcfSeenInSave(i))
            Unlocks_Mark(UNLOCK_CATEGORY_VIDEO, s_names[i]);
    }
}

int Videoteca_Total(void) {
    LoadNames();
    return s_total;
}

int Videoteca_SeenCount(void) {
    Unlocks_Load();
    int seen = 0;
    const int n = Unlocks_Count(UNLOCK_CATEGORY_VIDEO);
    for (int i = 0; i < n; i++) {
        if (IsKnown(Unlocks_Nth(UNLOCK_CATEGORY_VIDEO, i)))
            seen++;
    }
    return seen;
}

const char *Videoteca_SeenName(int index) {
    Unlocks_Load();
    const int n = Unlocks_Count(UNLOCK_CATEGORY_VIDEO);
    for (int i = 0; i < n; i++) {
        const char *name = Unlocks_Nth(UNLOCK_CATEGORY_VIDEO, i);
        if (!IsKnown(name))
            continue;
        if (index == 0)
            return name;
        index--;
    }
    return NULL;
}

/* Titles for a Spanish menu. The others show the cinematic's own name. */
static const struct {
    const char *name;
    const char *title;
} kSpanishTitles[] = {
    {"INTRO", "Introducción"},
    {"ASCENSEU", "El ascensor"},
    {"ASRETOUR", "El ascensor (regreso)"},
    {"BOAT1", "Barco 1"},
    {"BOAT2", "Barco 2"},
    {"BOAT3", "Barco 3"},
    {"BOAT4", "Barco 4"},
    {"CRASH", "El choque"},
    {"DELUGE", "El diluvio"},
    {"END", "Final"},
    {"END2", "Final (segunda parte)"},
    {"ENFA", "Los niños"},
    {"FRAGMENT", "El fragmento"},
    {"GROTTE", "La gruta"},
    {"LUNES1", "Las lunas 1"},
    {"LUNES2", "Las lunas 2"},
    {"MOON", "La luna"},
    {"PASSEUR", "El barquero"},
    {"PUB1", "Anuncio 1"},
    {"PUB2", "Anuncio 2"},
    {"PUB3", "Anuncio 3"},
    {"PUB4A6", "Anuncios 4 a 6"},
    {"SORT", "El hechizo"},
    {"SURSAUT", "El sobresalto"},
    {"TAXI", "Taxi"},
    {"TAXI_J", "Taxi (2)"},
    {"BABY", "Epílogo"},
};

static void Trim(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n'))
        s[--n] = '\0';
    size_t lead = 0;
    while (s[lead] == ' ' || s[lead] == '\t')
        lead++;
    if (lead)
        memmove(s, s + lead, n - lead + 1);
}

/* A title from video/hd/videoteca.txt, read every time so an edit shows the
   next time the library opens. */
static int TitleFromFile(const char *name, char *out, size_t size) {
    char path[ADELINE_MAX_PATH];
    GetMoviePath(path, ADELINE_MAX_PATH, "hd" ADELINE_PATH_SEP "videoteca.txt");
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    char line[256];
    int found = 0;
    while (!found && fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq || line[0] == '#')
            continue;
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;
        Trim(key);
        Trim(value);
        /* A UTF-8 byte order mark at the start of the file. */
        if ((unsigned char)key[0] == 0xEF && (unsigned char)key[1] == 0xBB && (unsigned char)key[2] == 0xBF)
            key += 3;
        if (value[0] && !ade_strcasecmp(key, name)) {
            snprintf(out, size, "%s", value);
            found = 1;
        }
    }
    fclose(f);
    return found;
}

void Videoteca_Title(const char *name, char *out, size_t size) {
    if (!out || !size)
        return;
    out[0] = '\0';
    if (!name)
        return;
    if (TitleFromFile(name, out, size))
        return;
    if (Language == 3) {
        for (size_t i = 0; i < sizeof(kSpanishTitles) / sizeof(kSpanishTitles[0]); i++) {
            if (!strcmp(kSpanishTitles[i].name, name)) {
                snprintf(out, size, "%s", kSpanishTitles[i].title);
                return;
            }
        }
    }
    /* "BOAT1" reads "Boat1". */
    size_t n = 0;
    for (; name[n] && n + 1 < size; n++) {
        char c = name[n];
        if (n > 0 && c >= 'A' && c <= 'Z')
            c = (char)(c - 'A' + 'a');
        out[n] = (c == '_') ? ' ' : c;
    }
    out[n] = '\0';
}

int Videoteca_HasHd(const char *name) {
    return Video_HasHdReplacement(name) ? 1 : 0;
}

void Videoteca_Play(const char *name) {
    S16 saved[ACF_FLAG_WORDS];
    AcfSaveFlags(saved);
    PlayAcf(name);
    AcfRestoreFlags(saved);
}
