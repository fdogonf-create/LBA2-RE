#ifndef HOLOHD_H
#define HOLOHD_H

#include <SYSTEM/ADELINE_TYPES.H>

/* Redrawn island maps for the holomap. A PNG named holomap_<entry>.png in the
 * `hd` folder beside the game data (entry: the map's index in holomap.hqr, as
 * scripts/holomap_export.py names them) is shown in place of the 640x480
 * indexed map, at the screen's own resolution, by a backend that can
 * (PlatformBackdrop); the arrows, Twinsen and the text stay drawn over it.
 * Without the file, or on a backend that cannot, the original map shows. */

/* Load the redrawn map `entry`, if there is one, for a map shown at (x0, y0)
 * in Log; `palette` (768 bytes) is the one it is shown in once faded in.
 * Nothing shows until HoloHD_SetView. */
extern void HoloHD_Begin(S32 entry, S32 x0, S32 y0, const U8 *palette);
/* The indexed map has just been drawn into its 640x480 rectangle of Log,
 * showing the part (sx, sy, sw, sh) of the original, and nothing is over it
 * yet: show the same part of the redrawn one there. */
extern void HoloHD_SetView(S32 sx, S32 sy, S32 sw, S32 sh);
/* Back to the indexed map alone. */
extern void HoloHD_End(void);

#endif
