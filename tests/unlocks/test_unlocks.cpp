/* The profile's unlock list (SOURCES/UNLOCKS.CPP): what it keeps, in which
   order, and that its file text reads back to the same list. */
#include "UNLOCKS.H"

#include <stdio.h>
#include <string.h>

static int g_failures = 0;

#define CHECK(cond)                                                \
    do {                                                           \
        if (!(cond)) {                                             \
            fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, \
                    __LINE__, #cond);                              \
            g_failures++;                                          \
        }                                                          \
    } while (0)

static void test_add_keeps_unlock_order_and_ignores_repeats(void) {
    Unlocks_Clear();
    CHECK(Unlocks_Add("video", "SORT") == 1);
    CHECK(Unlocks_Add("video", "INTRO.SMK") == 1);
    CHECK(Unlocks_Add("video", "intro") == 0);
    CHECK(Unlocks_Add("character", "ZOE") == 1);
    CHECK(Unlocks_Count("video") == 2);
    CHECK(Unlocks_Count("character") == 1);
    CHECK(!strcmp(Unlocks_Nth("video", 0), "SORT"));
    CHECK(!strcmp(Unlocks_Nth("video", 1), "INTRO"));
    CHECK(Unlocks_Nth("video", 2) == NULL);
    CHECK(Unlocks_Has("VIDEO", "Intro.smk"));
    CHECK(!Unlocks_Has("character", "INTRO"));
}

static void test_rejects_what_it_cannot_store(void) {
    Unlocks_Clear();
    CHECK(Unlocks_Add("video", "") == 0);
    CHECK(Unlocks_Add("", "INTRO") == 0);
    CHECK(Unlocks_Add("video", "A_NAME_FAR_TOO_LONG_TO_FIT_IN_THE_ENTRY") == 0);
    CHECK(Unlocks_Add(NULL, "INTRO") == 0);
    CHECK(Unlocks_Count("video") == 0);
}

static void test_format_reads_back_to_the_same_list(void) {
    char text[256];
    Unlocks_Clear();
    Unlocks_Add("video", "SORT");
    Unlocks_Add("character", "ZOE");
    Unlocks_Add("video", "BOAT1");
    const size_t need = Unlocks_Format(text, sizeof text);
    CHECK(need == strlen(text));
    CHECK(!strcmp(text, "video SORT\ncharacter ZOE\nvideo BOAT1\n"));

    Unlocks_Clear();
    Unlocks_Parse(text);
    CHECK(Unlocks_Count("video") == 2);
    CHECK(!strcmp(Unlocks_Nth("video", 1), "BOAT1"));
    CHECK(!strcmp(Unlocks_Nth("character", 0), "ZOE"));
}

static void test_format_cuts_at_a_whole_line(void) {
    char text[16];
    Unlocks_Clear();
    Unlocks_Add("video", "INTRO"); /* "video INTRO\n": 12 bytes */
    Unlocks_Add("video", "SORT");  /* 11 more: does not fit */
    Unlocks_Add("v", "A");         /* 4 more: would fit, must not be written */
    const size_t need = Unlocks_Format(text, sizeof text);
    CHECK(need == 12 + 11 + 4);
    CHECK(!strcmp(text, "video INTRO\n"));
}

static void test_parse_skips_comments_blanks_and_junk(void) {
    Unlocks_Clear();
    Unlocks_Parse("# seen\r\n\r\n  video   INTRO  \r\nnonsense\nvideo SORT");
    CHECK(Unlocks_Count("video") == 2);
    CHECK(!strcmp(Unlocks_Nth("video", 0), "INTRO"));
    CHECK(!strcmp(Unlocks_Nth("video", 1), "SORT"));
    CHECK(Unlocks_Count("NONSENSE") == 0);
}

int main(void) {
    test_add_keeps_unlock_order_and_ignores_repeats();
    test_rejects_what_it_cannot_store();
    test_format_reads_back_to_the_same_list();
    test_format_cuts_at_a_whole_line();
    test_parse_skips_comments_blanks_and_junk();
    if (g_failures) {
        fprintf(stderr, "test_unlocks: %d failure(s)\n", g_failures);
        return 1;
    }
    printf("test_unlocks: all passed\n");
    return 0;
}
