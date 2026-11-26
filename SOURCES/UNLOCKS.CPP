#include "UNLOCKS.H"

#include <stdio.h>
#include <string.h>

struct UnlockEntry {
    char category[UNLOCKS_MAX_CATEGORY];
    char name[UNLOCKS_MAX_NAME];
};

static UnlockEntry s_entries[UNLOCKS_MAX_ENTRIES];
static int s_count = 0;

static char UpperAscii(char c) {
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

/* Upper-case copy of a word, stopping at the extension. Returns 0 when the
   word is empty or too long to store. */
static int NormaliseWord(const char *src, char *dst, size_t size, int stopAtDot) {
    size_t n = 0;
    while (src[n] && !(stopAtDot && src[n] == '.')) {
        if (n + 1 >= size)
            return 0;
        dst[n] = UpperAscii(src[n]);
        n++;
    }
    dst[n] = '\0';
    return n > 0;
}

static int Find(const char *category, const char *name) {
    for (int i = 0; i < s_count; i++) {
        if (!strcmp(s_entries[i].category, category) && !strcmp(s_entries[i].name, name))
            return i;
    }
    return -1;
}

void Unlocks_Clear(void) {
    s_count = 0;
}

int Unlocks_Add(const char *category, const char *name) {
    char cat[UNLOCKS_MAX_CATEGORY];
    char nm[UNLOCKS_MAX_NAME];
    if (!category || !name)
        return 0;
    if (!NormaliseWord(category, cat, sizeof(cat), 0) || !NormaliseWord(name, nm, sizeof(nm), 1))
        return 0;
    if (Find(cat, nm) >= 0 || s_count >= UNLOCKS_MAX_ENTRIES)
        return 0;
    strcpy(s_entries[s_count].category, cat);
    strcpy(s_entries[s_count].name, nm);
    s_count++;
    return 1;
}

int Unlocks_Has(const char *category, const char *name) {
    char cat[UNLOCKS_MAX_CATEGORY];
    char nm[UNLOCKS_MAX_NAME];
    if (!category || !name)
        return 0;
    if (!NormaliseWord(category, cat, sizeof(cat), 0) || !NormaliseWord(name, nm, sizeof(nm), 1))
        return 0;
    return Find(cat, nm) >= 0;
}

int Unlocks_Count(const char *category) {
    char cat[UNLOCKS_MAX_CATEGORY];
    int n = 0;
    if (!category || !NormaliseWord(category, cat, sizeof(cat), 0))
        return 0;
    for (int i = 0; i < s_count; i++) {
        if (!strcmp(s_entries[i].category, cat))
            n++;
    }
    return n;
}

const char *Unlocks_Nth(const char *category, int index) {
    char cat[UNLOCKS_MAX_CATEGORY];
    if (!category || index < 0 || !NormaliseWord(category, cat, sizeof(cat), 0))
        return NULL;
    for (int i = 0; i < s_count; i++) {
        if (!strcmp(s_entries[i].category, cat)) {
            if (index == 0)
                return s_entries[i].name;
            index--;
        }
    }
    return NULL;
}

static int IsSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

void Unlocks_Parse(const char *text) {
    if (!text)
        return;
    while (*text) {
        const char *end = strchr(text, '\n');
        size_t len = end ? (size_t)(end - text) : strlen(text);
        char line[UNLOCKS_MAX_CATEGORY + UNLOCKS_MAX_NAME + 8];
        if (len < sizeof(line)) {
            memcpy(line, text, len);
            line[len] = '\0';
            char *p = line;
            while (IsSpace(*p))
                p++;
            if (*p && *p != '#') {
                char *cat = p;
                while (*p && !IsSpace(*p))
                    p++;
                if (*p) {
                    *p++ = '\0';
                    while (IsSpace(*p))
                        p++;
                    char *name = p;
                    while (*p && !IsSpace(*p))
                        p++;
                    *p = '\0';
                    Unlocks_Add(cat, name);
                }
            }
        }
        text += len;
        if (*text == '\n')
            text++;
    }
}

size_t Unlocks_Format(char *out, size_t size) {
    size_t total = 0;
    int fits = 1;
    if (out && size)
        out[0] = '\0';
    for (int i = 0; i < s_count; i++) {
        char category[UNLOCKS_MAX_CATEGORY];
        size_t c = 0;
        for (; s_entries[i].category[c]; c++) {
            char ch = s_entries[i].category[c];
            category[c] = (ch >= 'A' && ch <= 'Z') ? (char)(ch - 'A' + 'a') : ch;
        }
        category[c] = '\0';
        char line[UNLOCKS_MAX_CATEGORY + UNLOCKS_MAX_NAME + 4];
        int n = snprintf(line, sizeof(line), "%s %s\n", category, s_entries[i].name);
        if (n < 0)
            continue;
        /* Whole lines only: once one does not fit, nothing after it is
           written either, so a short buffer holds a clean prefix. */
        if (fits && out && total + (size_t)n < size)
            memcpy(out + total, line, (size_t)n + 1);
        else
            fits = 0;
        total += (size_t)n;
    }
    return total;
}
