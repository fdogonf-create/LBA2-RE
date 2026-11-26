#include "C_EXTERN.H"
#include "DIRECTORIES.H"
#include "HOLOHD.H"
#include "IMAGE_LOAD.H"
#include "INPUT.H"
#include "JOYSTICK.H"
#include "MENU_INPUT.H"

#include <FILEIO/SAVEPNG.H>
#include <SVGA/SCREEN.H>
#include <SYSTEM/KEYBOARD_KEYS.H>
#include <SYSTEM/LZ.H>
#include <string.h>
#include "AMBIANCE.H"

#define ZOOM_A 2048 //	Big Islands
#define ZOOM_B 1024 //	Islands with 4 cubes
#define ZOOM_C 768  //	Islands with 1 cube
#define ZOOM_D 256
#define ZOOM_E 128
#define ZOOM_F 96

S32 BodyFactorScale[MAX_ISLAND] =
    {
        ZOOM_A, //  0	Citadelle
        ZOOM_A, //  1	Puit Sendell
        ZOOM_A, //  2	Desert
        ZOOM_B, //  3	Lune
        ZOOM_A, //  4	Otringal
        ZOOM_C, //  5	Celebration
        ZOOM_B, //  6	Blafard
        ZOOM_B, //  7	Mosquibee
        ZOOM_B, //  8	Knartas
        ZOOM_C, //  9	Island CX
        ZOOM_C, // 10	Elevator
        ZOOM_B  // 11	Under Celebration
};

S32 ArrowFactorScale[MAX_ISLAND] =
    {
        ZOOM_D, //  0	Citadelle
        ZOOM_D, //  1	Puit Sendell
        ZOOM_D, //  2	Desert
        ZOOM_E, //  3	Lune
        ZOOM_D, //  4	Otringal
        ZOOM_F, //  5	Celebration
        ZOOM_E, //  6	Blafard
        ZOOM_E, //  7	Mosquibee
        ZOOM_E, //  8	Knartas
        ZOOM_F, //  9	Island CX
        ZOOM_F, // 10	Elevator
        ZOOM_E  // 11	Under Celebration
};
//----------------------------------------------------------------------------
void DrawTwinsenPlan(T_SORT *ptt) {
    SetClip(0, 0, ModeDesiredX - 1, Dial_Y0 - 1);

    ObjTwinsen.X = ptt->Xw;
    ObjTwinsen.Y = ptt->Yw;
    ObjTwinsen.Z = ptt->Zw;
    ObjTwinsen.Alpha = 0;
    ObjTwinsen.Beta = ListObjet[NUM_PERSO].Obj.Beta;
    ObjTwinsen.Gamma = 0;

    ObjectDisplay(&ObjTwinsen);

    BoxMovingAdd(ClipXMin, ClipYMin, ClipXMax, ClipYMax);
}
//────────────────────────────────────────────────────────────────────────────
/*
void	DrawObjFix(T_SORT *ptt)
{
	S16	ymin	;
	S16	ymax	;
	S16	*buf	;

	SetClip( 0, 0, ModeDesiredX-1, Dial_Y0-1 )	;

//	buf = (S16*)HQR_Get( HQR_ObjFix, ptt->NumObj );
	buf = (S16*)GivePtrObjFix( ptt->NumObj );
	if ( !buf )	return			;

	ymin = *(buf+3)	;
	ymax = *(buf+4)	;

	ptt->Yw -= ymin	;

	BodyDisplay( ptt->Xw, ptt->Yw , ptt->Zw, 0, TimerRef*10, 0, buf);

	BoxMovingAdd( ClipXMin, ClipYMin, ClipXMax, ClipYMax )	;

	ptt->Yw += ymax+1000	;//	Add ObjFix Size ZV YMax
}
*/
//----------------------------------------------------------------------------
void DrawArrowPlan(T_SORT *ptt) {
    T_BODY_HEADER *buf;
    S32 ymax;

    SetClip(0, 0, ModeDesiredX - 1, Dial_Y0 - 1);

    if (ptt->Num == 1)
        buf = (T_BODY_HEADER *)BufFleche; //	Big
    else
        buf = (T_BODY_HEADER *)BufLoFleche; //	Small

    ymax = buf->YMax;

    BodyDisplay(ptt->Xw, ptt->Yw, ptt->Zw, 0, 0, 0, buf);

    BoxMovingAdd(ClipXMin, ClipYMin, ClipXMax, ClipYMax);

    ptt->Yw += ymax + 1000; //	Add Arrow Size (ZV YMax)

    //	if ( ptt->NumObj != -1 )	DrawObjFix( ptt )	;// An object on me!

    if (ptt->Num == 2)
        DrawTwinsenPlan(ptt); // Twinsen is on me !
}
//----------------------------------------------------------------------------
void DrawBuggyPlan(T_SORT *ptt) {
    S32 a, b, g;

    SetClip(0, 0, ModeDesiredX - 1, Dial_Y0 - 1);

    a = ListBuggy[0].Alpha;
    b = ListBuggy[0].Beta;
    g = ListBuggy[0].Gamma;

    BodyDisplay(ptt->Xw, ptt->Yw + (TimerRefHR & 0xFF), ptt->Zw, a, b, g, BufBuggy);

    BoxMovingAdd(ClipXMin, ClipYMin, ClipXMax, ClipYMax);
}
//----------------------------------------------------------------------------
void DrawDynoPlan(T_SORT *ptt) {
    SetClip(0, 0, ModeDesiredX - 1, Dial_Y0 - 1);

    BodyDisplay(ptt->Xw, ptt->Yw + ((TimerRefHR & 0xF00) >> 8), ptt->Zw, 0, 0, 0, BufDyno);

    BoxMovingAdd(ClipXMin, ClipYMin, ClipXMax, ClipYMax);
}
//────────────────────────────────────────────────────────────────────────────
S32 RecupCoorTwinsen(S32 *xt, S32 *yt, S32 *zt) {
    S32 mode = 0;

    if (TabArrow[MAX_OBJECTIF + NumCube].FlagHolo & 4) // Exterieur
    {
        mode = 1;
    }

    if (TwinsenLinkArrow != -1) //	Twinsen link with an Arrow
    {
        *xt = TabArrow[TwinsenLinkArrow].X;
        *yt = TabArrow[TwinsenLinkArrow].Y;
        *zt = TabArrow[TwinsenLinkArrow].Z;
        return (mode);
    }

    if (mode == 1)                                               // Exterieur
    {                                                            //	Twinsen Outside
        *xt = ListObjet[NUM_PERSO].Obj.X + (CurrentCubeX * SCE); // TEMPO
        *yt = ListObjet[NUM_PERSO].Obj.Y;
        *zt = ListObjet[NUM_PERSO].Obj.Z + (CurrentCubeY * SCE);

        return (1); //	or mode, same thing
    }

    //	Twinsen Inside
    *xt = TabArrow[MAX_OBJECTIF + NumCube].X;
    *yt = TabArrow[MAX_OBJECTIF + NumCube].Y;
    *zt = TabArrow[MAX_OBJECTIF + NumCube].Z;

    return (0); //	or mode...
}
//────────────────────────────────────────────────────────────────────────────
/*
typedef struct {

	S32	X ;
	S32	Y ;
	S32	Z ;
	S32	Cube ;
	S32	Beta	;
	S32	Alpha	;
	S32     Gamma	;

	S_ONE_WHEEL	Wheel[4] ;

	S32		BetaWheel ;

	S32		SpeedInc ;
	S32		SpeedRot ;

	S32		Speed ;

//	S32		YSpeed ;
//	S32		LastYs ;
//	S32		LastSpeedLevel ;

	S32		LastTimer ;

		} S_BUGGY ;
*/

//	Le buggy ne peut se trouver que dans le cube 49 sur la citadelle,
//	et dans les cubes 55 a 73 sur le desert ( dans ce cas, on l'affiche ).
S32 RecupCoorBuggy(S32 *xb, S32 *yb, S32 *zb) {
    S32 cube, cx, cz;

    if ((!NumBuggy)
            OR(ListObjet[0].WorkFlags & MANUAL_INTER_FRAME)) { //	Never Init or Twinsen IN Buggy
        return (0);
    }

    cube = ListBuggy[0].Cube;

    if ((cube < 55) OR(cube > 73))
        return (0);

    cx = (7 + (cube - 54) / 5) * SCE;
    cz = (7 + (cube - 54) % 5) * SCE;

    *xb = ListBuggy[0].X + cx;
    *yb = ListBuggy[0].Y;
    *zb = ListBuggy[0].Z + cz;

    return (1);
}
//────────────────────────────────────────────────────────────────────────────
#define MAX_DYNO_POS 4
typedef struct
{
    S32 Ile;
    S32 X;
    S32 Y;
    S32 Z;
    S32 Beta;

} T_DYNO_POS;
T_DYNO_POS TabDynoPos[MAX_DYNO_POS] =
    {
        0, 10784 + (8 * SCE), 2000, 14640 + (10 * SCE), 1024, // Citadelle Twinsen, 49
        0, 16114 + (6 * SCE), 7250, 26473 + (8 * SCE), 3072,  // Citadelle Dome, 44
        2, 9234 + (7 * SCE), 850, 7235 + (8 * SCE), 3072,     // Desert Oasis, 55
        2, 22203 + (10 * SCE), 4791, 13591 + (11 * SCE), 431  // Desert Ilot, 73
};
//────────────────────────────────────────────────────────────────────────────
S32 RecupCoorDyno(S32 *xd, S32 *yd, S32 *zd) {
    S32 dyno;
#ifdef LBA_EDITOR
    dyno = ListVarGame[FLAG_DINO_VOYAGE].Value - 1;
#else
    dyno = ListVarGame[FLAG_DINO_VOYAGE] - 1;
#endif
    if ((dyno < 0) OR(dyno >= MAX_DYNO_POS))
        return (0);
    if (ZoomedIsland != TabDynoPos[dyno].Ile)
        return (0);

    *xd = TabDynoPos[dyno].X;
    *yd = TabDynoPos[dyno].Y;
    *zd = TabDynoPos[dyno].Z;

    return (1); //	TempO
}
//────────────────────────────────────────────────────────────────────────────
void DrawListHoloPlan() {
    S32 xt, yt, zt;
    S32 xb, yb, zb;
    S32 xd, yd, zd;
    S32 alt;
    T_ARROW *pt;
    T_SORT *ptt;
    S32 arrow;
    S32 nbtri = 0;
    U32 timer;
    U32 angle;

    timer = TimerRefHR * 2;

    /*	NumObjectif is 1000 when Twinsen is the objective and not linked to an
        arrow, and -1 when there is none. Neither is a slot in DecalTimerRef,
        which DrawListHoloGlobe guards the same way. */
    if ((NumObjectif != OldObjectif) AND(NumObjectif >= 0)
            AND(NumObjectif < MAX_OBJECTIF + MAX_CUBE)
                AND(DecalTimerRef[NumObjectif] == 3072)) {
        DecalTimerRef[NumObjectif] -= timer;
    }

    ptt = ListTri;

    pt = &TabArrow[MAX_OBJECTIF];

    for (arrow = MAX_OBJECTIF; arrow < MAX_OBJECTIF + MAX_CUBE; arrow++, pt++) {
        if ((!(pt->FlagHolo & 1))
                OR(pt->Island != ZoomedIsland))
            continue;

        alt = pt->Y;

        if (arrow == NumObjectif) {
            angle = (DecalTimerRef[arrow] + timer) & 4095;
        } else {
            angle = DecalTimerRef[arrow] & 4095;

            if (angle != 3072) {
                angle = (angle + timer) & 4095;

                // angle-3072 results in long unsigned int
                // but interpreting that an implicit cast to
                // int is happening since this is the signature
                // of the abs function in Watcom lib.
                int tmpFix = angle - 3072;
                if (abs(tmpFix) < 64)
                    DecalTimerRef[arrow] = 3072;
            }
        }

        alt += 2048 + ((SinTab[angle]) >> 3);

        if (TwinsenLinkArrow == arrow)
            ptt->Num = 2; // Small Arrow
        else
            ptt->Num = 1; // Big Arrow

        LongWorldRotatePoint(pt->X, alt, pt->Z);

        ptt->NumObj = pt->ObjFix;
        ptt->Z = Z0;
        ptt->Xw = pt->X;
        ptt->Yw = alt;
        ptt->Zw = pt->Z;
        ptt++;
        nbtri++;
    }

    //----- Here I must (eventually) add twinsen to ListTri...

    if ((TwinsenLinkArrow == -1)     //		Not on an arrow
        AND(ZoomedIsland == Island)) //	Twinsen is here!
    {
        RecupCoorTwinsen(&xt, &yt, &zt);
        LongWorldRotatePoint(xt, yt, zt);
        ptt->Num = 0; //	Type Twinsen
        ptt->Z = Z0;
        ptt->Xw = xt;
        ptt->Yw = yt;
        ptt->Zw = zt;
        ptt++;
        nbtri++;
    }

    //----- Here I must (eventually) add the Buggy to ListTri...

    if ((ZoomedIsland == 2)                 //	Separe cause Debug
        AND(RecupCoorBuggy(&xb, &yb, &zb))) //	Desert + Buggy
    {
        LongWorldRotatePoint(xb, yb, zb);
        ptt->Num = 3; //	Type Buggy
        ptt->Z = Z0;
        ptt->Xw = xb;
        ptt->Yw = yb;
        ptt->Zw = zb;
        ptt++;
        nbtri++;
    }

    //----- Here I must (eventually) add the Dyno to ListTri...

    if (RecupCoorDyno(&xd, &yd, &zd)) //	Desert ou Citadelle
    {
        LongWorldRotatePoint(xd, yd, zd);
        ptt->Num = 4; //	Type Dyno
        ptt->Z = Z0;
        ptt->Xw = xd;
        ptt->Yw = yd;
        ptt->Zw = zd;
        ptt++;
        nbtri++;
    }

    //----- Now we sort all thoose stuffs

    qsort(ListTri, nbtri, sizeof(T_SORT), z_sort32);

    ptt = ListTri;

    for (arrow = 0; arrow < nbtri; arrow++, ptt++) {
        switch (ptt->Num) {
        case 0:
            DrawTwinsenPlan(ptt);
            break;
        case 1:
        case 2:
            DrawArrowPlan(ptt);
            break;
        case 3:
            DrawBuggyPlan(ptt);
            break;
        case 4:
            DrawDynoPlan(ptt);
            break;
        }
    }
}
//════════════════════════════════════════════════════════════════════════════
/* Community additions to the island map: any island Twinsen knows can be
   opened (from the globe, and with L / R from one map to the next; only to
   look, he stays where he is), the right stick zooms in on the chosen place,
   and the island's name is written at the top. */
#define HOLO_ZOOM_ONE 1024
#define HOLO_ZOOM_MAX 3072
#define HOLO_ZOOM_RATE 2048 /* per second at full tilt: 1x to 3x in a second */

static U8 s_mapClean[IMAGE_AUTHORED_WIDTH * IMAGE_AUTHORED_HEIGHT]; /* the map as loaded */
static S32 s_mapX0 = 0, s_mapY0 = 0;                                /* where it sits in Log */
static S32 s_zoom = HOLO_ZOOM_ONE;
static S32 s_viewX = 0, s_viewY = 0, s_viewW = IMAGE_AUTHORED_WIDTH, s_viewH = IMAGE_AUTHORED_HEIGHT;

static const char *const s_islandNamesEs[MAX_ISLAND] = {
    "Isla de la Ciudadela", "Pozo de Sendell", "Isla del Desierto", "Luna Esmeralda",
    "Otringal", "Isla de la Celebraci\xA2n", "Isla de los Wannies", "Isla de los Mosquibees",
    "Knartas", "Isla CX", "Ascensor", "Bajo la Isla de la Celebraci\xA2n"};
static const char *const s_islandNamesEn[MAX_ISLAND] = {
    "Citadel Island", "Sendell's Well", "Desert Island", "Emerald Moon",
    "Otringal", "Celebration Island", "Island of the Wannies", "Island of the Mosquibees",
    "Knartas", "Island CX", "Elevator", "Under Celebration Island"};

/* Whether holomap.hqr has a map for `island` (Sendell's well has none). */
static S32 HoloPlanHasMap(S32 island) {
    static S8 s_hasMap[MAX_ISLAND] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    if (s_hasMap[island] < 0) {
        char path[ADELINE_MAX_PATH];
        GetResPath(path, ADELINE_MAX_PATH, HOLO_HQR_NAME);
        s_hasMap[island] = (S8)(HQF_ResSize(path, HQR_BEGIN_MAP + 2 * island) >=
                                IMAGE_AUTHORED_WIDTH * IMAGE_AUTHORED_HEIGHT);
    }
    return s_hasMap[island];
}

S32 HoloPlan_IslandKnown(S32 island) {
    S32 i;
    if (island < 0 OR island >= MAX_ISLAND)
        return FALSE;
    if (island != Island AND !HoloPlanHasMap(island))
        return FALSE;
    if (island == Island)
        return TRUE;
    for (i = MAX_OBJECTIF; i < MAX_OBJECTIF + MAX_CUBE; i++) {
        if ((TabArrow[i].FlagHolo & 1) AND TabArrow[i].Island == island)
            return TRUE;
    }
    return FALSE;
}

static void HoloPlanSetProjection(void) {
    SetProjection(s_mapX0 + (IMAGE_AUTHORED_WIDTH / 2 - s_viewX) * s_zoom / HOLO_ZOOM_ONE,
                  s_mapY0 + (IMAGE_AUTHORED_HEIGHT / 2 - s_viewY) * s_zoom / HOLO_ZOOM_ONE, 1024,
                  700 * s_zoom / HOLO_ZOOM_ONE, 700 * s_zoom / HOLO_ZOOM_ONE);
}

/* Draw the map at the current zoom, centred as near (fx, fy) (map pixels) as
   its edges allow, with the name and the brackets, into Log and Screen. */
static void HoloPlanDrawMap(S32 fx, S32 fy) {
    const S32 x1 = s_mapX0 + IMAGE_AUTHORED_WIDTH - 1;
    const S32 y1 = s_mapY0 + IMAGE_AUTHORED_HEIGHT - 1;
    S32 x, y;

    s_viewW = IMAGE_AUTHORED_WIDTH * HOLO_ZOOM_ONE / s_zoom;
    s_viewH = IMAGE_AUTHORED_HEIGHT * HOLO_ZOOM_ONE / s_zoom;
    s_viewX = fx - s_viewW / 2;
    s_viewY = fy - s_viewH / 2;
    if (s_viewX > IMAGE_AUTHORED_WIDTH - s_viewW)
        s_viewX = IMAGE_AUTHORED_WIDTH - s_viewW;
    if (s_viewY > IMAGE_AUTHORED_HEIGHT - s_viewH)
        s_viewY = IMAGE_AUTHORED_HEIGHT - s_viewH;
    if (s_viewX < 0)
        s_viewX = 0;
    if (s_viewY < 0)
        s_viewY = 0;

    for (y = 0; y < IMAGE_AUTHORED_HEIGHT; y++) {
        const U8 *src = s_mapClean + (size_t)(s_viewY + y * s_viewH / IMAGE_AUTHORED_HEIGHT) * IMAGE_AUTHORED_WIDTH;
        U8 *dst = (U8 *)Log + (size_t)(s_mapY0 + y) * ModeDesiredX + s_mapX0;
        for (x = 0; x < IMAGE_AUTHORED_WIDTH; x++)
            dst[x] = src[s_viewX + x * s_viewW / IMAGE_AUTHORED_WIDTH];
    }
    HoloHD_SetView(s_viewX, s_viewY, s_viewW, s_viewH);

    if (ZoomedIsland >= 0 AND ZoomedIsland < MAX_ISLAND) {
        char *name = (char *)(Language == 3 ? s_islandNamesEs : s_islandNamesEn)[ZoomedIsland];
        const S32 nx = s_mapX0 + (IMAGE_AUTHORED_WIDTH - SizeFont(name)) / 2;
        UnsetClip();
        ColorFont(BLACK);
        Font(nx + 2, s_mapY0 + 14, name);
        ColorFont(LBAWHITE);
        Font(nx, s_mapY0 + 12, name);
    }

    AffUpperLeft(s_mapX0, s_mapY0, 0);
    AffLowerLeft(s_mapX0, y1, 0);
    AffUpperRight(x1, s_mapY0, 0);
    AffLowerRight(x1, y1, 0);

    CopyScreen(Log, Screen);
    HoloPlanSetProjection();
}

/* Where on the unzoomed map the chosen place is, to zoom in on it. */
static void HoloPlanFocus(S32 *fx, S32 *fy) {
    if (NumObjectif == -1) {
        *fx = s_viewX + s_viewW / 2;
        *fy = s_viewY + s_viewH / 2;
        return;
    }
    *fx = s_viewX + (XpObjectif - s_mapX0) * HOLO_ZOOM_ONE / s_zoom;
    *fy = s_viewY + (YpObjectif - s_mapY0) * HOLO_ZOOM_ONE / s_zoom;
}

void InitHoloPlan() {
    /*	M3D  36 octets, 9 LONG. Load_HQR lands a compressed entry at
        ptrdest + SizeFile - CompressedSizeFile + RECOVER_AREA, so the
        destination has to hold SizeFile + RECOVER_AREA. Most islands store
        this record compressed; without the margin the read runs ~470 bytes
        past the array and over the saved registers of the callers. */
    S32 buffer[(36 + RECOVER_AREA) / sizeof(S32)];
    S32 *pt;
    S32 orgmx, orgmz;
    S32 offx, offz;
    S32 alpha, beta;
    S32 lalpha, lbeta;
    S32 distance;
    S32 id;
    char tmpFilePath[ADELINE_MAX_PATH];

    InitHoloMalloc();

    BufBodyTempo = (U8 *)HoloMalloc(SizeCurrentBody);
    BufAnimTempo = (U8 *)HoloMalloc(SizeCurrentAnim);

    if ((ZoomedIsland == 0) AND(TEMPETE_FINIE)) {
        id = HQR_BEGIN_MAP + (2 * 12); //	Citabau
    }
#ifdef LBA_EDITOR
    else if ((ZoomedIsland == 5) AND(ListVarGame[FLAG_CELEBRATION].Value))
#else
    else if ((ZoomedIsland == 5) AND(ListVarGame[FLAG_CELEBRATION]))
#endif
    {
        id = HQR_BEGIN_MAP + (2 * 13); //	Celebra2
    } else
        id = HQR_BEGIN_MAP + (2 * ZoomedIsland);

    GetResPath(tmpFilePath, ADELINE_MAX_PATH, HOLO_HQR_NAME);

    // The planet-detail HQR entry is a 640×480 raw bitmap. A raw Load_HQR
    // smears it diagonally into a wider Log; Image_LoadCentered blits it
    // centred with proper row stride. Clear Log first so the sidebars are
    // black instead of showing whatever HoloGlobe left behind.
    memset(Log, 0, (size_t)ModeDesiredX * (size_t)ModeDesiredY);
    Image_LoadCentered(tmpFilePath, Log, id);

    Load_HQR(tmpFilePath, buffer, id + 1);
    FromLE32InPlace(buffer, 9); // the camera record: nine S32, little-endian

    // Anchor UI to the *image* rect, not the framebuffer rect. The corner
    // brackets sit on the image corners; the 3D projection used for arrows
    // + Twinsen + Buggy/Dyno targets the image centre so models line up
    // with the texture features (citadel, arrows, etc.).
    const S32 imageX0 = ((S32)ModeDesiredX - IMAGE_AUTHORED_WIDTH) / 2;
    const S32 imageY0 = ((S32)ModeDesiredY - IMAGE_AUTHORED_HEIGHT) / 2;
    const S32 imageX1 = imageX0 + IMAGE_AUTHORED_WIDTH - 1;
    const S32 imageY1 = imageY0 + IMAGE_AUTHORED_HEIGHT - 1;

    /* A redrawn map in the hd folder, if there is one. */
    HoloHD_Begin(id, imageX0, imageY0, PtrPal);

    s_mapX0 = imageX0;
    s_mapY0 = imageY0;
    for (S32 row = 0; row < IMAGE_AUTHORED_HEIGHT; row++)
        memcpy(s_mapClean + (size_t)row * IMAGE_AUTHORED_WIDTH,
               (U8 *)Log + (size_t)(imageY0 + row) * ModeDesiredX + imageX0, IMAGE_AUTHORED_WIDTH);
    s_zoom = HOLO_ZOOM_ONE;
    HoloPlanDrawMap(IMAGE_AUTHORED_WIDTH / 2, IMAGE_AUTHORED_HEIGHT / 2);
    (void)imageX1;
    (void)imageY1;

    //	To avoid Zoom with Window
    if (!FlagMap)
        HoloInitDialWindow();

    pt = buffer;

    orgmx = *pt++;
    orgmz = *pt++;
    offx = *pt++;
    offz = *pt++;
    alpha = *pt++;
    beta = *pt++;
    distance = *pt++;
    lalpha = *pt++;
    lbeta = *pt++;

    orgmx = (orgmx * SCE) + offx;
    orgmz = (orgmz * SCE) + offz;

    BufFleche = LoadHoloBody_HQR(tmpFilePath, HQR_FLECHE);
    BufLoFleche = LoadHoloBody_HQR(tmpFilePath, HQR_LOFLECHE);
    BufBuggy = LoadHoloBody_HQR(tmpFilePath, HQR_BUGGY);
    BufDyno = LoadHoloBody_HQR(tmpFilePath, HQR_DYNO);

    ScaleRotBody3D(BufFleche, ArrowFactorScale[ZoomedIsland], FALSE);
    ScaleRotBody3D(BufLoFleche, ArrowFactorScale[ZoomedIsland], FALSE);

    ScaleRotBody3D(BufBuggy, SCALE_BUGGY_PLAN, FALSE);
    ScaleRotBody3D(BufDyno, SCALE_DYNO_PLAN, FALSE);

    //	SetProjection( 320, 240, 1024, 1024, 1024 ) 			;
    HoloPlanSetProjection();
    SetFollowCamera(orgmx, 0, orgmz, alpha, beta, 0, distance);
    SetLightVector(lalpha, lbeta, 0);

    //------------------------------
    //----- Recuperation body current de Twinsen et size dans HQR
    memcpy(BufBodyTempo, PtCurrentBody, SizeCurrentBody);
    memcpy(BufAnimTempo, PtCurrentAnim, SizeCurrentAnim);

    ScaleRotBody3D(BufBodyTempo, BodyFactorScale[ZoomedIsland], FALSE);
    ScaleRotAnim3D(BufAnimTempo, BodyFactorScale[ZoomedIsland], FALSE);

    ObjectClear(&ObjTwinsen);

    ObjectInitBody(&ObjTwinsen, 0);
    ObjectInitAnim(&ObjTwinsen, 0);
    //------------------------------

    NumOldMess = -1;
}
//════════════════════════════════════════════════════════════════════════════
void ClearHoloPlan() {
    HoloHD_End();
    /*      Unused depuis HoloMalloc

	if ( BufDyno )		Free(BufDyno)		;
	BufDyno = 0L					;

	if ( BufBuggy )		Free(BufBuggy)		;
	BufBuggy = 0L					;

	if( BufLoFleche )	Free(BufLoFleche)	;
	BufLoFleche = NULL	;

	if( BufFleche )		Free(BufFleche)		;
	BufFleche = NULL ;
*/
}
//════════════════════════════════════════════════════════════════════════════
//	Oblige to found one, because user can zoom on this island
void SearchFirstValidObjectifIsland() {
    S32 i;
    T_ARROW *pt;

    pt = &TabArrow[MAX_OBJECTIF];

    for (i = MAX_OBJECTIF; i < MAX_OBJECTIF + MAX_CUBE; i++, pt++) {
        if ((pt->FlagHolo & 1)
                AND(pt->Island == ZoomedIsland)) {
            NumObjectif = i;
            return;
        }
    }
}
//════════════════════════════════════════════════════════════════════════════
//	Oblige to found one, because user can zoom on this island
S32 FindNearestOutsideArrow(S32 xt, S32 yt, S32 zt) {
    S32 i;
    T_ARROW *pt;
    S32 min = 9999;
    S32 dist;
    S32 arrow = -1;

    pt = &TabArrow[MAX_OBJECTIF];

    for (i = MAX_OBJECTIF; i < MAX_OBJECTIF + MAX_CUBE; i++, pt++) {
        if ((pt->FlagHolo & 1)
                AND(pt->Island == ZoomedIsland)) {
            dist = Distance3D(pt->X, pt->Y, pt->Z, xt, yt, zt);
            if (dist < min) {
                arrow = i;
                dist = min;
            }
        }
    }

    if ((arrow != -1)
            AND(dist <= DISTANCE_EXT))
        return (arrow);

    return (-1);
}
//════════════════════════════════════════════════════════════════════════════

void InitObjectifIsland() {
    S32 xt, yt, zt;
    S32 arrow;
    T_ARROW *pt;

    NumObjectif = -1;      //	None by default
    OldObjectif = -2;      //	For AffReticule
    TwinsenLinkArrow = -1; //	Alone by default

    if (ZoomedIsland == Island) //	Twinsen is here = Objectif
    {
        if (RecupCoorTwinsen(&xt, &yt, &zt)) //	Outside
        {
            arrow = FindNearestOutsideArrow(xt, yt, zt);

            if (arrow != -1) {
                NumObjectif = arrow;
                TwinsenLinkArrow = arrow;
            } else
                NumObjectif = 1000; // Twinsen
        } else                      // 	Inside
        {
            if (TabArrow[MAX_OBJECTIF + NumCube].FlagHolo & 1) {
                NumObjectif = MAX_OBJECTIF + NumCube;
                TwinsenLinkArrow = NumObjectif;
            } else {
                NumObjectif = 1000; // Twinsen
            }
        }

    } else
        SearchFirstValidObjectifIsland();

    if (NumObjectif != 1000) {
        pt = &TabArrow[NumObjectif];
        xt = pt->X;
        yt = pt->Y;
        zt = pt->Z;
    }

    LongWorldRotatePoint(xt, yt, zt);
    LongProjectPoint(X0, Y0, Z0);

    XpObjectif = Xp;
    YpObjectif = Yp;

    SizeRet = 16; //	Size Reticule First Time
}
//════════════════════════════════════════════════════════════════════════════
void InitAngleArrow() {
    S32 arrow;

    for (arrow = 0; arrow < MAX_OBJECTIF + MAX_ARROW; arrow++) {
        DecalTimerRef[arrow] = 3072;
    }
}
//════════════════════════════════════════════════════════════════════════════
//	0:Left  1:Up  2:Right  3:Down
void SearchObjectifIsland(S32 dir) {
    T_ARROW *pt;
    S32 i, next;
    S32 x, y, dd;
    //	S32	dx, dy	;
    S32 min = 9999;
    S32 xt, yt, zt;
    S32 flag = 0;

    next = -1;

    pt = &TabArrow[MAX_OBJECTIF];

    //----- Scan Objectif Arrow

    for (i = MAX_OBJECTIF; i < MAX_OBJECTIF + MAX_CUBE; i++, pt++) {
        if ((!(pt->FlagHolo & 1))
                OR(i == NumObjectif)
                    OR(pt->Island != ZoomedIsland))
            continue;

        LongWorldRotatePoint(pt->X, pt->Y, pt->Z);
        LongProjectPoint(X0, Y0, Z0);

        //		dx = abs(Xp-XpObjectif)	;
        //		dy = abs(Yp-YpObjectif)	;

        dd = Distance2D(Xp, Yp, XpObjectif, YpObjectif);

        if (dir & 1) //	Left
        {
            if (Xp > XpObjectif)
                continue;
            //			if ( dy > dx )		continue	;
        }
        if (dir & 2) //	Up
        {
            if (Yp > YpObjectif)
                continue;
            //			if ( dx > dy )		continue	;
        }
        if (dir & 4) //	Right
        {
            if (Xp < XpObjectif)
                continue;
            //			if ( dy > dx )		continue	;
        }
        if (dir & 8) //	Down
        {
            if (Yp < YpObjectif)
                continue;
            //			if ( dx > dy )		continue	;
        }

        if (dd < min) {
            min = dd;
            next = i;
            x = Xp;
            y = Yp;
        }
    }

    if ((ZoomedIsland == Island)     //	Twinsen is here!
        AND(NumObjectif != 1000)     //	He is not the current objectif
        AND(TwinsenLinkArrow == -1)) //	And he isn't linked with an arrow
    {
        RecupCoorTwinsen(&xt, &yt, &zt);

        LongWorldRotatePoint(xt, yt, zt);
        LongProjectPoint(X0, Y0, Z0);

        //		dx = abs(Xp-XpObjectif)	;
        //		dy = abs(Yp-YpObjectif)	;

        dd = Distance2D(Xp, Yp, XpObjectif, YpObjectif);

        if (dir & 1) //	Left
        {
            if (Xp > XpObjectif)
                flag = 1;
            //			if ( dy > dx )		flag = 1	;
        }
        if (dir & 2) //	Up
        {
            if (Yp > YpObjectif)
                flag = 1;
            //			if ( dx > dy )		flag = 1	;
        }
        if (dir & 4) //	Right
        {
            if (Xp < XpObjectif)
                flag = 1;
            //			if ( dy > dx )		flag = 1	;
        }
        if (dir & 8) //	Down
        {
            if (Yp < YpObjectif)
                flag = 1;
            //			if ( dx > dy )		flag = 1	;
        }

        if ((!flag)AND(dd < min)) {
            min = dd;
            next = 1000; //	Twinsen
            x = Xp;
            y = Yp;
        }
    }

    if (next == -1)
        return; //	Keep the same

    NumObjectif = next;
    XpObjectif = x;
    YpObjectif = y;
}
//════════════════════════════════════════════════════════════════════════════
/* Harness capture state — see HoloPlan_RequestCapture() in HOLO.H.  When
   the path is non-empty, HoloPlan's modal loop fires a one-shot SavePNG
   once the post-fade steady state is reached, then sets FlagHoloEnd and
   breaks so HoloPlan exits cleanly. */
static char s_holoplan_capture_path[ADELINE_MAX_PATH] = "";
static int s_holoplan_capture_countdown = 0;

void HoloPlan_RequestCapture(const char *path) {
    if (!path || !path[0]) {
        s_holoplan_capture_path[0] = '\0';
        s_holoplan_capture_countdown = 0;
        return;
    }
    strncpy(s_holoplan_capture_path, path, ADELINE_MAX_PATH - 1);
    s_holoplan_capture_path[ADELINE_MAX_PATH - 1] = '\0';
    /* The zoom-in inner loop runs for ~1000 ms of in-game time when FlagMap
       is set; the FadeToPal pass after the first composite is another few
       frames.  40 outer iterations at fixed-dt 16 ms gives the zoom + fade
       time to settle before we grab the frame. */
    s_holoplan_capture_countdown = 40;
}

/* Queried from HoloMap so the same harness pre-arm (texture-filler globals
   etc.) that fires for HoloGlobe captures also fires when we drive a
   HoloPlan-only capture through HoloMap (HoloMode=1 path).  Without this,
   PtrMap/ObjPtrMap stay stale on the first tick after --load and the
   textured planet body segfaults inside Filler_TextureGouraud. */
int HoloPlan_IsCaptureArmed(void) {
    return s_holoplan_capture_path[0] != '\0';
}

/* L / R (Page Up / Page Down) step to the previous / next island Twinsen
   knows; the right stick zooms in and out on the chosen place. */
static void HoloPlanExtraInput(void) {
    static S32 s_prevL = 0, s_prevR = 0;
    static U32 s_lastTime = 0;
    static S32 s_lastObjectif = -2;
    const S32 l = CheckKey(K_GAMEPAD_LSHOULDER) OR CheckKey(K_PAGE_UP);
    const S32 r = CheckKey(K_GAMEPAD_RSHOULDER) OR CheckKey(K_PAGE_DOWN);
    const U32 now = TimerRefHR;
    U32 dt = now - s_lastTime;
    S32 step = 0;
    S32 zoom = s_zoom;
    S32 fx, fy;

    s_lastTime = now;
    if (dt > 100)
        dt = 100;
    if (l AND !s_prevL)
        step = -1;
    else if (r AND !s_prevR)
        step = 1;
    s_prevL = l;
    s_prevR = r;

    if (step) {
        S32 i, island = ZoomedIsland;
        for (i = 0; i < MAX_ISLAND; i++) {
            island = (island + step + MAX_ISLAND) % MAX_ISLAND;
            if (HoloPlan_IslandKnown(island))
                break;
        }
        if (island != ZoomedIsland) {
            ClearHoloPlan();
            ZoomedIsland = island;
            InitHoloPlan();
            InitObjectifIsland();
            InitAngleArrow();
            OldObjectif = -2;
            NumOldMess = -1;
            BoxStaticAdd(0, 0, (S32)ModeDesiredX - 1, (S32)ModeDesiredY - 1);
            s_lastObjectif = NumObjectif;
            return;
        }
    }

    if (JoystickIsPresent()) {
        S16 rx = 0, ry = 0;
        JoystickGetRightStick(&rx, &ry);
        if (ry<-JoystickDeadzone OR ry> JoystickDeadzone)
            zoom -= (S32)((S64)ry * HOLO_ZOOM_RATE * (S32)dt / (32768 * 1000));
    }
    if (zoom < HOLO_ZOOM_ONE)
        zoom = HOLO_ZOOM_ONE;
    if (zoom > HOLO_ZOOM_MAX)
        zoom = HOLO_ZOOM_MAX;

    /* Zoomed in, a new place chosen with the D-pad brings the view to it. */
    if (zoom != s_zoom OR(s_zoom != HOLO_ZOOM_ONE AND NumObjectif != s_lastObjectif)) {
        HoloPlanFocus(&fx, &fy);
        s_zoom = zoom;
        HoloPlanDrawMap(fx, fy);
        BoxStaticAdd(0, 0, (S32)ModeDesiredX - 1, (S32)ModeDesiredY - 1);
    }
    s_lastObjectif = NumObjectif;
}

void HoloPlan(S32 numplan) {
    S32 x0, y0, x1, y1;
    S32 testrelease = 0;
    S32 testnofire = TRUE;
    //	U32	mkey=0, mfire=0	;
    //	U32	mjoy=0, dir	;
    U32 dir, timer;

    UnsetClip();

    ZoomedIsland = numplan;

    HoloMode = 1;

    InitHoloPlan();

    InitObjectifIsland();

    InitAngleArrow();

    if (!FlagMap)
        BoxStaticAdd(0, 0, (S32)ModeDesiredX - 1, (S32)ModeDesiredY - 1); //	Full Screen

    while (!FlagHoloEnd) {
        ManageTime();
        MyGetInput();

        if ((MyKey == K_ESC)
                OR(Input & (I_MENUS | I_HOLOMAP))) {
            FlagHoloEnd = TRUE;
            break;
        }

        if (!FlagMap)
            HoloPlanExtraInput();

        if (testrelease) {
            if (!MenuInput_Held()) {
                testrelease = 0;
            }
        } else if (testnofire) {
            if (!(Input & I_FIRE))
                testnofire = FALSE;
        } else if (!FlagMap) {
            if ((Input & I_ACTION_M) OR(Input & I_RETURN)) {
                break;
            }

            if (MenuInput_Get()) {
                dir = 0;

                if (MenuInput_Get() & MENU_INPUT_LEFT)
                    dir |= 1;
                if (MenuInput_Get() & MENU_INPUT_UP)
                    dir |= 2;
                if (MenuInput_Get() & MENU_INPUT_RIGHT)
                    dir |= 4;
                if (MenuInput_Get() & MENU_INPUT_DOWN)
                    dir |= 8;

                if (dir) {
                    SearchObjectifIsland(dir);
                    testrelease = 1;
                }
            }
        }

        DoAnimHolo();

        DrawListHoloPlan();

        if (FlagMap) {
            CopyScreen(Log, BufSpeak);
            UnsetClip();
            MemoTimerRot = TimerRefHR;

            // pour virer résidus non restaurés venants de
            //      HoloGlobe()
            BoxReset();

            // Zoom-in animates from the reticule centre (CENTERX/Y are
            // image-relative) out to the image rect, not the framebuffer
            // rect. Only the growing box is marked, so everything outside
            // it, sidebars included, keeps the last globe frame until the
            // full flip below, as in the original.
            const S32 anim_imageX0 = ((S32)ModeDesiredX - IMAGE_AUTHORED_WIDTH) / 2;
            const S32 anim_imageY0 = ((S32)ModeDesiredY - IMAGE_AUTHORED_HEIGHT) / 2;
            const S32 anim_imageX1 = anim_imageX0 + IMAGE_AUTHORED_WIDTH - 1;
            const S32 anim_imageY1 = anim_imageY0 + IMAGE_AUTHORED_HEIGHT - 1;
            while (TRUE) {
                ManageTime();
                timer = TimerRefHR - MemoTimerRot;
                x0 = BoundRegleTrois(CENTERX + anim_imageX0 - 8, anim_imageX0, 1000, timer);
                y0 = BoundRegleTrois(CENTERY + anim_imageY0 - 8, anim_imageY0, 1000, timer);
                x1 = BoundRegleTrois(CENTERX + anim_imageX0 + 8, anim_imageX1, 1000, timer);
                y1 = BoundRegleTrois(CENTERY + anim_imageY0 + 8, anim_imageY1, 1000, timer);

                if (x0 == anim_imageX0) {
                    break; // Before Crash!
                }

                ScaleBox(anim_imageX0, anim_imageY0, anim_imageX1, anim_imageY1, BufSpeak, x0, y0, x1, y1, Log);

                AffUpperLeft(x0, y0, 0);
                AffLowerLeft(x0, y1, 0);
                AffUpperRight(x1, y0, 0);
                AffLowerRight(x1, y1, 0);

                BoxStaticAdd(x0, y0, x1, y1); // Optimisable
                BoxUpdate();
            }
            CopyScreen(BufSpeak, Log);
            HoloInitDialWindow();
            BoxStaticFullflip();
            FlagMap = 0;
            //	To avoid Jumping Arrow
            SetTimerHR(MemoTimerRot);
            InitWaitNoInput(I_FIRE);
        }

        UnsetClip();

        AffReticule();

        AffHoloMess();

#ifndef LBA_EDITOR
        CheckSaveLogPcx(PtrPal); // F9: Sauve Screen
#endif

        BoxUpdate();

        OldObjectif = NumObjectif;

        if (FlagFade) // Fade après 1er affichage
        {
            MemoTimerRot = TimerRefHR;
            FadeToPal(PtrPal);
            SetTimerHR(MemoTimerRot);
            FlagFade = FALSE;
        }

        /* Harness one-shot capture.  Sits at the bottom of the modal loop so
           the captured Log buffer contains the post-fade, post-zoom-in steady
           frame: corner brackets at the four screen extents, planet body
           centred via SetProjection, and any reticule + dialogue strip the
           player would see at rest. */
        if (s_holoplan_capture_path[0] && --s_holoplan_capture_countdown <= 0) {
            SavePNG(s_holoplan_capture_path, Log, (S32)ModeDesiredX,
                    (S32)ModeDesiredY, PtrPal);
            s_holoplan_capture_path[0] = '\0';
            FlagHoloEnd = TRUE;
            break;
        }

        //-------------
        /*UnsetClip()					;
CoulText( 255, 0 )				;
Text( 0, 0, "%F%D     ", NbFramePerSecond )	;
CmptFrame++					;
*/
    }

    ClearHoloPlan();

    InitWaitNoInput(I_RETURN | I_FIRE | I_MENUS | I_HOLOMAP);
}
//----------------------------------------------------------------------------
