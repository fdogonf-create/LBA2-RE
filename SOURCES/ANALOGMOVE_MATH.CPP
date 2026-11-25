/*--------------------------------------------------------------------------*/
/* Angle math for analog movement. No engine state: see ANALOGMOVE.H. */
#include "ANALOGMOVE.H"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static S32 RadiansToBeta(double rad) {
    S32 beta = (S32)floor(rad * (4096.0 / (2.0 * M_PI)) + 0.5);
    return beta & 4095;
}

S32 AnalogMove_AngleDiff(S32 from, S32 to) {
    S32 d = (to - from) & 4095;
    if (d >= 2048)
        d -= 4096;
    return d;
}

S32 AnalogMove_StepToward(S32 beta, S32 target, S32 maxStep) {
    S32 d = AnalogMove_AngleDiff(beta, target);
    if (maxStep < 0)
        maxStep = 0;
    if (d > maxStep)
        d = maxStep;
    else if (d < -maxStep)
        d = -maxStep;
    return (beta + d) & 4095;
}

S32 AnalogMove_StickAngle(S32 x, S32 y) {
    /* Up is -y; a turn to the left is positive, so left (-x) maps to +1024. */
    return RadiansToBeta(atan2((double)-x, (double)-y));
}

S32 AnalogMove_TargetExterior(S32 x, S32 y, S32 betaCam) {
    return ((2048 - betaCam) + AnalogMove_StickAngle(x, y)) & 4095;
}

S32 AnalogMove_TargetIso(S32 x, S32 y) {
    /* LongProjectPointIso: Xp grows with 3(x-z)/64, Yp with 3(x+z)/128, so one
     * screen unit up costs twice the world distance of one unit across. */
    double a = (double)x;         /* screen right */
    double b = 2.0 * (double)(-y); /* screen up, scaled to match */
    double wx = (a - b) * 0.5;
    double wz = (-a - b) * 0.5;
    return RadiansToBeta(atan2(wx, wz));
}
