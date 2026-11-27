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

/* The file is looked for beside the game data and in the user folder, also as
 * tester_camara.txt.txt (Windows hiding the extension), every few seconds until
 * found, so it can be added while the game runs. */
extern S32 CamHud_Enabled(void);

/* Who drives the camera this frame. Anything but AUTO shows only a line saying
 * so and where Twinsen is, so the readout always tells the tester it is on. */
#define CAM_HUD_AUTO 0
#define CAM_HUD_ZONE 1
#define CAM_HUD_INTERIOR 2
#define CAM_HUD_CINEMA 3
#define CAM_HUD_OFF 4

/* Called every game frame, before CamHud_Note can be. */
extern void CamHud_NoteMode(U32 nowMs, S32 mode, S32 island, S32 cube);

/* This frame's values. `nowMs` is the platform clock; the readout goes away
 * a little after the last frame that noted values. */
extern void CamHud_Note(U32 nowMs, S32 alpha, S32 alphaMin, S32 alphaMax, S32 dist, S32 distMin, S32 distMax,
                        S32 beta, S32 island, S32 cube);

/* Draw it over the presented frame (overlay pre-present). */
extern void CamHud_Render(U32 *frameBuffer, U32 pitch, U32 width, U32 height, const U8 *palette_rgb);

#endif
