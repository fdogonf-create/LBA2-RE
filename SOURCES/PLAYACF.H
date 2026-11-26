#ifndef PLAYACF_H
#define PLAYACF_H

#ifdef __cplusplus
extern "C" {
#endif

//▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
// COPY.ASM

//▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
typedef void Func_CopyFrameDoubleXY(void *src);
/*
#pragma aux Arg_CopyFrameDoubleXY	"*"	\
	parm caller	[esi]			\
	modify		[eax ebx ecx edx edi]

#pragma	aux (Arg_CopyFrameDoubleXY)	Func_CopyFrameDoubleXY
	*/
//▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
extern Func_CopyFrameDoubleXY *CopyFrameDoubleXY;

//#pragma	aux	CopyFrameDoubleXY	"*"

//▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀

#ifdef __cplusplus
}
#endif

// PLAYACF.CPP

//▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
extern char *ListAcf; /* the cinematics' names, as RESS.HQR lists them */
extern void InitAcf(void);
extern void ResetAcfList(void);
//▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
extern char *GetAcfName(char *name);
//▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
extern S32 GetNumAcf(const char *name);
//▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
extern void PlayAllAcf(void);
//▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀
/* Contract: PlayAcf fades the video out and leaves the screen BLACK on return
   (SetBlackPal, FlagBlackPal=TRUE). The caller must reveal the next screen --
   in-game via FlagFade + AffScene (LM_PLAY_ACF / TM_PLAY_ACF etc.), or by its own
   FadeToPal (menu win-screen, console preview). A caller that neither fades in nor
   restores will sit on black. */
extern S32 PlayAcf(const char *name);
/* Harness hook: arm a one-shot screenshot inside the next PlayAcf call.
   PlayAcf saves the Log buffer to <path> on the first composited cinema
   frame and exits cleanly via its fin_play label (the normal restore of
   the pre-cutscene scene runs unchanged afterwards). Pass NULL or "" to
   disarm. No-op in null-video-backend builds. Used by the console
   `ui video <name> <path>` verb. */
extern void Video_RequestCapture(const char *path);

/* Whether video/hd/ holds a high-definition replacement for this cinematic.
   Always false in a build without movie playback. */
extern S32 Video_HasHdReplacement(const char *name);

/* The save's record of which cinematics it has played: one bit per ListAcf
   number, in ACF_FLAG_WORDS game variables from FLAG_ACF. */
#define ACF_FLAG_WORDS 3
extern S32 AcfSeenInSave(S32 n);
extern void AcfSaveFlags(S16 *saved);
extern void AcfRestoreFlags(const S16 *saved);
/* Records as seen every cinematic any save in the save folder has played. */
extern void AcfNoteSeenFromSaves(void);

#ifdef USE_MVIDEO_SMACKER
void VideoLogEnable(int enable);
int VideoLogIsEnabled(void);
#endif

#endif // PLAYACF_H
