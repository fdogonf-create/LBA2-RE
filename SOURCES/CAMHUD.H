#ifndef CAMHUD_H
#define CAMHUD_H

#include <SYSTEM/ADELINE_TYPES.H>

/* TEMPORARY tester readout of the Auto camera, to choose its limits by eye.
 * While a file named tester_camara.txt sits beside the game data (sdmc:/lba2/
 * on the Switch), the top-left corner shows the camera's height (AlphaCam),
 * its distance (FollowCamBaseDist, what zoom sets), its turn and where
 * Twinsen is, each with the range it may take. It shows only while the
 * outdoor Auto camera is the one in use. Meant to be removed once the limits
 * are set. */

/* The file is looked for once, at the first call. */
extern S32 CamHud_Enabled(void);

/* This frame's values. `nowMs` is the platform clock; the readout goes away
 * a little after the last frame that noted values. */
extern void CamHud_Note(U32 nowMs, S32 alpha, S32 alphaMin, S32 alphaMax, S32 dist, S32 distMin, S32 distMax,
                        S32 beta, S32 island, S32 cube);

/* Draw it over the presented frame (overlay pre-present). */
extern void CamHud_Render(U32 *frameBuffer, U32 pitch, U32 width, U32 height, const U8 *palette_rgb);

#endif
