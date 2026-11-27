#ifndef HOLOHD_H
#define HOLOHD_H

#include <SYSTEM/ADELINE_TYPES.H>

/* Redrawn island maps for the holomap. A PNG named holomap_<entry>.png in the
 * `hd` folder beside the game data (entry: the map's index in holomap.hqr, as
 * scripts/holomap_export.py names them) is shown in place of the 640x480
 * indexed map, at the screen's own resolution, by a backend that can
 * (PlatformBackdrop); the arrows, Twinsen and the text stay drawn over it.
 * A picture wider than 4:3 (a widescreen map) has the map in its middle 4:3,
 * which stands where the original does; the rest shows in the screen's side
 * bars. Without the file, or on a backend that cannot, the original map shows. */

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

/* The globe's surface: a 256-texel-wide indexed texture (`size` bytes) from
 * holomap.hqr entry `entry`. If hd/hologlobe_<entry>.png exists, it is scaled
 * to the texture's size and each pixel replaced by the nearest of the colours
 * the original texture uses (`palette`, 768 bytes), so the redrawn surface
 * keeps working in the game's 256 colours. */
extern void HoloHD_ReplaceTexture(S32 entry, U8 *texture, U32 size, const U8 *palette);

#endif
