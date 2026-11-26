#ifndef HOLOHD_H
#define HOLOHD_H

#include <SYSTEM/ADELINE_TYPES.H>

/* Redrawn island maps for the holomap. A PNG named holomap_<entry>.png in the
 * `hd` folder beside the game data (entry: the map's index in holomap.hqr, as
 * scripts/holomap_export.py names them) is shown in place of the 640x480
 * indexed map, at the screen's own resolution, by a backend that can
 * (PlatformBackdrop); the arrows, Twinsen and the text stay drawn over it.
 * Without the file, or on a backend that cannot, the original map shows. */

/* After the indexed map `entry` has been drawn into Log with its top-left at
 * (x0, y0), and before anything is drawn over it. `palette` (768 bytes) is the
 * one the map is shown in once faded in. */
extern void HoloHD_Begin(S32 entry, S32 x0, S32 y0, const U8 *palette);
/* Back to the indexed map alone. */
extern void HoloHD_End(void);

#endif
