/* Analog movement angle math (SOURCES/ANALOGMOVE_MATH.CPP). */
#include "test_harness.h"

#include "ANALOGMOVE.H"

#include <math.h>

#define FULL 32767

static void test_angle_diff(void) {
    ASSERT_EQ_INT(0, AnalogMove_AngleDiff(100, 100));
    ASSERT_EQ_INT(100, AnalogMove_AngleDiff(0, 100));
    ASSERT_EQ_INT(-100, AnalogMove_AngleDiff(100, 0));
    /* The short way round across the wrap. */
    ASSERT_EQ_INT(20, AnalogMove_AngleDiff(4086, 10));
    ASSERT_EQ_INT(-20, AnalogMove_AngleDiff(10, 4086));
    ASSERT_EQ_INT(-2048, AnalogMove_AngleDiff(0, 2048));
}

static void test_step_toward(void) {
    ASSERT_EQ_INT(50, AnalogMove_StepToward(0, 1000, 50));
    ASSERT_EQ_INT(1000, AnalogMove_StepToward(990, 1000, 50));
    ASSERT_EQ_INT(4046, AnalogMove_StepToward(0, 3000, 50));
    ASSERT_EQ_INT(10, AnalogMove_StepToward(4086, 10, 100));
    ASSERT_EQ_INT(7, AnalogMove_StepToward(7, 500, 0));
}

static void test_stick_angle(void) {
    ASSERT_EQ_INT(0, AnalogMove_StickAngle(0, -FULL));    /* up */
    ASSERT_EQ_INT(1024, AnalogMove_StickAngle(-FULL, 0)); /* left */
    ASSERT_EQ_INT(2048, AnalogMove_StickAngle(0, FULL));  /* down */
    ASSERT_EQ_INT(3072, AnalogMove_StickAngle(FULL, 0));  /* right */
    ASSERT_EQ_INT(512, AnalogMove_StickAngle(-FULL, -FULL));
}

/* Camera behind a hero facing `beta` sits at BetaCam = 2048 - beta (INTEXT.CPP),
 * so stick up must keep him facing the way he already does. */
static void test_exterior(void) {
    S32 beta;
    for (beta = 0; beta < 4096; beta += 256) {
        S32 betaCam = (2048 - beta) & 4095;
        ASSERT_EQ_INT(beta, AnalogMove_TargetExterior(0, -FULL, betaCam));
        ASSERT_EQ_INT((beta + 1024) & 4095, AnalogMove_TargetExterior(-FULL, 0, betaCam));
        ASSERT_EQ_INT((beta + 2048) & 4095, AnalogMove_TargetExterior(0, FULL, betaCam));
        ASSERT_EQ_INT((beta + 3072) & 4095, AnalogMove_TargetExterior(FULL, 0, betaCam));
    }
}

/* Walk one step at the target facing (0 = +Z, 1024 = +X), project it the way
 * LongProjectPointIso does, and check the screen motion points where the stick
 * does. */
static void IsoScreenStep(S32 beta, double *sx, double *sy) {
    double a = (double)beta * (2.0 * 3.14159265358979323846 / 4096.0);
    double wx = sin(a) * 1000.0;
    double wz = cos(a) * 1000.0;
    *sx = 3.0 * (wx - wz) / 64.0;
    *sy = 6.0 * (wx + wz) / 256.0; /* screen Y grows downward */
}

static void check_iso(S32 x, S32 y) {
    double sx, sy;
    double want, got, d;
    IsoScreenStep(AnalogMove_TargetIso(x, y), &sx, &sy);
    want = atan2((double)y, (double)x);
    got = atan2(sy, sx);
    d = fabs(want - got);
    if (d > 3.14159265358979323846)
        d = 2.0 * 3.14159265358979323846 - d;
    ASSERT_TRUE(d < 0.01);
}

static void test_iso(void) {
    ASSERT_EQ_INT(2560, AnalogMove_TargetIso(0, -FULL)); /* screen up: world -X-Z */
    ASSERT_EQ_INT(1536, AnalogMove_TargetIso(FULL, 0));  /* screen right: world +X-Z */
    check_iso(0, -FULL);
    check_iso(0, FULL);
    check_iso(FULL, 0);
    check_iso(-FULL, 0);
    check_iso(FULL, -FULL);
    check_iso(-FULL, FULL);
    check_iso(12000, -25000);
    check_iso(-30000, -7000);
}

int main(void) {
    RUN_TEST(test_angle_diff);
    RUN_TEST(test_step_toward);
    RUN_TEST(test_stick_angle);
    RUN_TEST(test_exterior);
    RUN_TEST(test_iso);
    TEST_SUMMARY();
    return test_failures != 0;
}
