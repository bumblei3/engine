/*
 * tests_qmath6.c - Unit tests for the remaining standalone math/color
 * helpers in qcommon/q_math.c that were not yet covered by the existing
 * suites (tests_qmath.c .. tests_qmath5.c):
 *
 *   - ClampChar / ClampShort  (saturating clamps; WRAP on overflow)
 *   - VectorNormalize2        (returns length, writes unit vector)
 *   - Vector4Scale            (4-component scale)
 *   - AngleMod                (fixed-point mod 360)
 *   - AngleSubtract / AnglesSubtract (always in [-180,180])
 *   - LerpAngle               (shortest-path angle lerp)
 *   - ColorBytes3 / ColorBytes4 (float->packed byte color)
 *   - SetPlaneSignbits        (cplane_t.signbits from normal)
 *   - Q_rand / Q_random / Q_crandom (deterministic PRNG)
 *
 * All are pure (no engine state, no SDL/GL), so they link standalone with
 * q_shared.c + q_math.c + -lm.
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qmath6.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_all
 */

#include "oa_test.h"

#include "q_shared.h"

/* ------------------------------------------------------------------ */
/* ClampChar / ClampShort                                             */
/* ------------------------------------------------------------------ */

TEST(clampchar_in_range)
{
	/* values strictly inside the signed-char range pass through */
	OA_ASSERT_INT(ClampChar(0), 0);
	OA_ASSERT_INT(ClampChar(1), 1);
	OA_ASSERT_INT(ClampChar(-1), -1);
	OA_ASSERT_INT(ClampChar(127), 127);
	OA_ASSERT_INT(ClampChar(-128), -128);
}

TEST(clampchar_overflow_clamps_not_wraps)
{
	/* The engine SATURATES, it does NOT wrap. Assert real behaviour. */
	OA_ASSERT_INT(ClampChar(128), 127);
	OA_ASSERT_INT(ClampChar(255), 127);
	OA_ASSERT_INT(ClampChar(100000), 127);
	OA_ASSERT_INT(ClampChar(-129), -128);
	OA_ASSERT_INT(ClampChar(-100000), -128);
}

TEST(clampshort_in_range)
{
	OA_ASSERT_INT(ClampShort(0), 0);
	OA_ASSERT_INT(ClampShort(32767), 32767);
	OA_ASSERT_INT(ClampShort(-32768), -32768);
}

TEST(clampshort_overflow_clamps_not_wraps)
{
	/* Saturating, not wrapping (assert real engine behaviour). */
	OA_ASSERT_INT(ClampShort(32768), 32767);
	OA_ASSERT_INT(ClampShort(0x7fffffff), 32767);
	OA_ASSERT_INT(ClampShort(-32769), -32768);
	OA_ASSERT_INT(ClampShort((int)0x80000000), -32768);
}

/* ------------------------------------------------------------------ */
/* VectorNormalize2                                                   */
/* ------------------------------------------------------------------ */

TEST(vectornormalize2_unit_result)
{
	vec3_t v = { 3.0f, 4.0f, 0.0f };
	vec3_t out;
	vec3_t expect = { 0.6f, 0.8f, 0.0f };
	float len = VectorNormalize2(v, out);
	/* length returned is the euclidean length (5) */
	OA_ASSERT_FLOAT(len, 5.0f, 1e-4f);
	/* out is the unit vector */
	OA_ASSERT_VEC3(out, expect, 1e-4f);
}

TEST(vectornormalize2_zero)
{
	vec3_t v = { 0.0f, 0.0f, 0.0f };
	vec3_t out;
	vec3_t expect = { 0.0f, 0.0f, 0.0f };
	float len = VectorNormalize2(v, out);
	/* zero-length returns 0; the engine writes v[i]*ilength(=1/sqrt(0)) = 0,
	 * so out becomes the zero vector (NOT left untouched). */
	OA_ASSERT_FLOAT(len, 0.0f, 1e-6f);
	OA_ASSERT_VEC3(out, expect, 1e-6f);
}

/* ------------------------------------------------------------------ */
/* Vector4Scale                                                       */
/* ------------------------------------------------------------------ */

TEST(vector4scale_basic)
{
	vec4_t in = { 1.0f, -2.0f, 3.0f, 4.0f };
	vec4_t out;
	Vector4Scale(in, 2.0f, out);
	OA_ASSERT_FLOAT(out[0], 2.0f, 1e-6f);
	OA_ASSERT_FLOAT(out[1], -4.0f, 1e-6f);
	OA_ASSERT_FLOAT(out[2], 6.0f, 1e-6f);
	OA_ASSERT_FLOAT(out[3], 8.0f, 1e-6f);
}

/* ------------------------------------------------------------------ */
/* AngleMod / AngleSubtract / AnglesSubtract                         */
/* ------------------------------------------------------------------ */

TEST(anglemod_basic)
{
	/* AngleMod uses the SAME fixed-point formula as AngleNormalize360,
	 * so a 370 input maps to ~10 (not exactly: 9.99756), tolerance 1e-2. */
	OA_ASSERT_FLOAT(AngleMod(370.0f), 10.0f, 1e-2f);
	OA_ASSERT_FLOAT(AngleMod(360.0f), 0.0f, 1e-2f);
	OA_ASSERT_FLOAT(AngleMod(-30.0f), 330.0f, 1e-2f);
}

TEST(anglesubtract_normalizes_to_180)
{
	/* AngleSubtract returns a value in (-180, 180]. */
	OA_ASSERT_FLOAT(AngleSubtract(10.0f, 20.0f), -10.0f, 1e-3f);
	OA_ASSERT_FLOAT(AngleSubtract(350.0f, 10.0f), -20.0f, 1e-3f);
	OA_ASSERT_FLOAT(AngleSubtract(10.0f, 350.0f), 20.0f, 1e-3f);
	/* a difference of exactly 180 is inclusive at the +180 end */
	OA_ASSERT_FLOAT(AngleSubtract(180.0f, 0.0f), 180.0f, 1e-3f);
}

TEST(anglesubtract_vec3)
{
	vec3_t v1 = { 10.0f, 350.0f, 45.0f };
	vec3_t v2 = { 20.0f, 10.0f, 45.0f };
	vec3_t v3;
	AnglesSubtract(v1, v2, v3);
	OA_ASSERT_FLOAT(v3[0], -10.0f, 1e-3f);
	OA_ASSERT_FLOAT(v3[1], -20.0f, 1e-3f);
	OA_ASSERT_FLOAT(v3[2], 0.0f, 1e-3f);
}

/* ------------------------------------------------------------------ */
/* LerpAngle (shortest-path)                                          */
/* ------------------------------------------------------------------ */

TEST(lerpangle_shortest_path)
{
	/* from 350 to 10: the endpoints are wrapped to the shortest path
	 * (to += 360 -> 370), then linearly interpolated WITHOUT re-normalizing
	 * the result, so 0.5 step yields 360 (not 0). */
	float m = LerpAngle(350.0f, 10.0f, 0.5f);
	OA_ASSERT_FLOAT(m, 360.0f, 1e-3f);
	/* endpoints */
	OA_ASSERT_FLOAT(LerpAngle(0.0f, 90.0f, 0.0f), 0.0f, 1e-3f);
	OA_ASSERT_FLOAT(LerpAngle(0.0f, 90.0f, 1.0f), 90.0f, 1e-3f);
	OA_ASSERT_FLOAT(LerpAngle(0.0f, 90.0f, 0.5f), 45.0f, 1e-3f);
}

/* ------------------------------------------------------------------ */
/* ColorBytes3 / ColorBytes4                                          */
/* ------------------------------------------------------------------ */

TEST(colorbytes3_layout)
{
	/* 1.0 -> 255 in the low 3 bytes, alpha byte untouched (0). */
	unsigned c = ColorBytes3(1.0f, 0.0f, 0.5f);
	byte *p = (byte *)&c;
	OA_ASSERT_INT(p[0], 255);
	OA_ASSERT_INT(p[1], 0);
	/* round(0.5*255) = 127 or 128 depending on rounding mode */
	OA_ASSERT(p[2] == 127 || p[2] == 128);
}

TEST(colorbytes4_layout)
{
	unsigned c = ColorBytes4(1.0f, 0.0f, 0.5f, 0.0f);
	byte *p = (byte *)&c;
	OA_ASSERT_INT(p[0], 255);
	OA_ASSERT_INT(p[1], 0);
	OA_ASSERT_INT(p[2], 127);
	OA_ASSERT_INT(p[3], 0);
}

/* ------------------------------------------------------------------ */
/* SetPlaneSignbits                                                   */
/* ------------------------------------------------------------------ */

TEST(setplanesignbits_positive_normal)
{
	cplane_t p;
	Com_Memset(&p, 0, sizeof(p));
	p.normal[0] = 1.0f; p.normal[1] = 1.0f; p.normal[2] = 1.0f;
	SetPlaneSignbits(&p);
	/* all components >= 0 -> no sign bits set */
	OA_ASSERT_INT(p.signbits, 0);
}

TEST(setplanesignbits_negative_normal)
{
	cplane_t p;
	Com_Memset(&p, 0, sizeof(p));
	p.normal[0] = -1.0f; p.normal[1] = 0.0f; p.normal[2] = -1.0f;
	SetPlaneSignbits(&p);
	/* component j negative -> bit (1<<j) set */
	OA_ASSERT_INT(p.signbits, (1 << 0) | (1 << 2));
}

/* ------------------------------------------------------------------ */
/* Q_rand / Q_random / Q_crandom (deterministic PRNG)                */
/* ------------------------------------------------------------------ */

TEST(qrand_deterministic)
{
	/* Q_rand is a pure LCG: same seed -> same sequence. */
	int s1 = 12345, s2 = 12345;
	OA_ASSERT_INT(Q_rand(&s1), Q_rand(&s2));
	int a = Q_rand(&s1), b = Q_rand(&s2);
	OA_ASSERT_INT(a, b);
}

TEST(qrand_range_and_seed_advance)
{
	int seed = 1;
	int v = Q_rand(&seed);
	/* LCG returns the updated seed, which is always positive (no & mask) */
	OA_ASSERT(v > 0);
	/* the seed variable is advanced in place */
	OA_ASSERT_INT(seed, (69069 * 1 + 1));
}

TEST(qrandom_in_unit_interval)
{
	int seed = 99;
	float r = Q_random(&seed);
	/* Q_random = (Q_rand & 0xffff) / 0x10000  -> [0,1) */
	OA_ASSERT(r >= 0.0f && r < 1.0f);
}

TEST(qcrandom_centered)
{
	int seed = 99;
	float r = Q_crandom(&seed);
	/* Q_crandom = 2*(random-0.5) -> (-1,1) */
	OA_ASSERT(r > -1.0f && r < 1.0f);
}
