#ifndef UNLOCKS_H
#define UNLOCKS_H

#include <stddef.h>

/* Things the player has unlocked, kept across saves and sessions: one list
   for the whole profile, not one per save. Each entry is a category ("video"
   for a cinematic seen in the story) and a name within it, held in the order
   they were first unlocked, so a gallery can list them in the order the
   player met them.

   The list half (Unlocks_Add .. Unlocks_Format) has no engine dependency, so
   the host test links it on its own. Unlocks_Load / Unlocks_Mark keep it in
   a text file beside the saves, one "<category> <name>" per line. */

#define UNLOCKS_MAX_ENTRIES 512
#define UNLOCKS_MAX_CATEGORY 16
#define UNLOCKS_MAX_NAME 32

#define UNLOCK_CATEGORY_VIDEO "video"

/* Empties the list. */
void Unlocks_Clear(void);

/* Adds an entry. Names compare without case and are stored upper case; a
   name's extension ("INTRO.SMK") is dropped. Returns 1 if it was new, 0 if
   it was already there or could not be stored. */
int Unlocks_Add(const char *category, const char *name);

int Unlocks_Has(const char *category, const char *name);

/* Entries of one category, in unlock order. */
int Unlocks_Count(const char *category);
const char *Unlocks_Nth(const char *category, int index);

/* Adds every entry of a text in the file format. Blank lines and lines
   starting with '#' are skipped, as is anything malformed. */
void Unlocks_Parse(const char *text);

/* Writes the whole list in the file format. Returns the length the text
   needs, not counting the terminator; it is cut short when that is not less
   than size. */
size_t Unlocks_Format(char *out, size_t size);

/* Reads the file, once; later calls do nothing. */
void Unlocks_Load(void);

/* Unlock_Add, then write the file when the entry was new. */
void Unlocks_Mark(const char *category, const char *name);

#endif /* UNLOCKS_H */
