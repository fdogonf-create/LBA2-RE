#ifndef VIDEOTECA_H
#define VIDEOTECA_H

#include <stddef.h>

/* The movie library reached from the main menu: every cinematic the player
   has seen in the story, in the order they first saw it, to watch again.
   What has been seen lives in the profile's unlock list (UNLOCKS.H), so it
   survives starting a new game or loading another save. */

/* Records every cinematic the loaded save has already played. A save made
   before the library existed brings what it had seen this way. */
void Videoteca_NoteSeenFromSave(void);

/* Cinematics the game has, and how many of them have been seen. */
int Videoteca_Total(void);
int Videoteca_SeenCount(void);

/* The i-th seen cinematic, in the order it was first seen. */
const char *Videoteca_SeenName(int index);

/* The title shown for a cinematic, in UTF-8. A line "NAME = Title" in
   video/hd/videoteca.txt gives a cinematic its own. */
void Videoteca_Title(const char *name, char *out, size_t size);

/* Whether a high-definition replacement plays for this cinematic. */
int Videoteca_HasHd(const char *name);

/* Plays a cinematic from the menu without touching the game in progress:
   the save's own record of which cinematics it has played stays as it was.
   Leaves the screen black, as PlayAcf does. */
void Videoteca_Play(const char *name);

#endif /* VIDEOTECA_H */
