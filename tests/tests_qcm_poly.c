/*
 * tests_qcm_poly.c - Unit tests for the convex-winding geometry helpers in
 * qcommon/cm_polylib.c (collision-map polygon math). These are pure geometry
 * (no CM world state) and link standalone once AllocWinding/FreeWinding are
 * backed by malloc (see test_stubs_cm.c). They exercise the real engine
 * winding code paths used by the BSP collision routines.
 *
 *   - BaseWindingForPlane  (square winding for an axis-aligned plane)
 *   - WindingArea / WindingBounds / WindingCenter / WindingPlane
 *   - CopyWinding
 *   - WindingOnPlaneSide
 *   - ChopWindingInPlace
 *   - RemoveColinearPoints
 *
 * Build & run (separate binary, with cm_polylib.c + Z-stubs):
 *   cd engine/tests
 *   gcc -I. -I../code/qcommon -I../code -o test_cm \
 *       oa_test_run.c test_main.c test_stubs_cm.c \
 *       tests_qcm_poly.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/cm_polylib.c -lm
 *   ./test_cm
 */

#include "oa_test.h"

#include "q_shared.h"
#include "cm_polylib.h"

/* Build a 2x2 square winding in the XY plane (z=0) for assertions. */
static winding_t *make_square_xy(void)
{
	winding_t *w = AllocWinding(4);
	w->numpoints = 4;
	w->p[0][0] = -1.0f; w->p[0][1] = -1.0f; w->p[0][2] = 0.0f;
	w->p[1][0] =  1.0f; w->p[1][1] = -1.0f; w->p[1][2] = 0.0f;
	w->p[2][0] =  1.0f; w->p[2][1] =  1.0f; w->p[2][2] = 0.0f;
	w->p[3][0] = -1.0f; w->p[3][1] =  1.0f; w->p[3][2] = 0.0f;
	return w;
}

TEST(basewindingforplane_unit_square)
{
	/* Plane normal +Z, dist 0 -> a 2x2 square winding centred at origin. */
	vec3_t n = { 0.0f, 0.0f, 1.0f };
	winding_t *w = BaseWindingForPlane(n, 0.0f);
	OA_ASSERT_INT(w->numpoints, 4);
	vec3_t expect = { 0.0f, 0.0f, 0.0f };
	vec3_t center;
	WindingCenter(w, center);
	OA_ASSERT_VEC3(center, expect, 1e-3f);
	FreeWinding(w);
}

TEST(windingarea_square)
{
	winding_t *w = make_square_xy();
	/* 2x2 square -> area 4 */
	OA_ASSERT_FLOAT(WindingArea(w), 4.0f, 1e-3f);
	FreeWinding(w);
}

TEST(windingbounds_square)
{
	winding_t *w = make_square_xy();
	vec3_t mins, maxs;
	WindingBounds(w, mins, maxs);
	OA_ASSERT_FLOAT(mins[0], -1.0f, 1e-3f);
	OA_ASSERT_FLOAT(mins[1], -1.0f, 1e-3f);
	OA_ASSERT_FLOAT(maxs[0],  1.0f, 1e-3f);
	OA_ASSERT_FLOAT(maxs[1],  1.0f, 1e-3f);
	OA_ASSERT_FLOAT(maxs[2],  0.0f, 1e-3f);
	FreeWinding(w);
}

TEST(windingcenter_square)
{
	winding_t *w = make_square_xy();
	vec3_t center, expect = { 0.0f, 0.0f, 0.0f };
	WindingCenter(w, center);
	OA_ASSERT_VEC3(center, expect, 1e-3f);
	FreeWinding(w);
}

TEST(windingplane_square)
{
	winding_t *w = make_square_xy();
	vec3_t normal, expect = { 0.0f, 0.0f, -1.0f };
	vec_t dist;
	WindingPlane(w, normal, &dist);
	/* The engine computes CrossProduct(v2, v1) (reversed argument order),
	 * so the plane normal of an XY square winds to -Z, not +Z. */
	OA_ASSERT_VEC3(normal, expect, 1e-3f);
	OA_ASSERT_FLOAT(dist, 0.0f, 1e-3f);
	FreeWinding(w);
}

TEST(copywinding_identical)
{
	winding_t *w = make_square_xy();
	winding_t *c = CopyWinding(w);
	OA_ASSERT_INT(c->numpoints, w->numpoints);
	vec3_t ac, bc;
	WindingCenter(w, ac);
	WindingCenter(c, bc);
	OA_ASSERT_VEC3(ac, bc, 1e-6f);
	FreeWinding(w);
	FreeWinding(c);
}

TEST(windingonplaneside_in_front)
{
	winding_t *w = make_square_xy();  /* z = 0 plane */
	/* Plane z = -10 (normal +Z, dist -10): d = 0 - (-10) = +10 > EPS, so the
	 * whole winding is in FRONT of the plane. */
	vec3_t n = { 0.0f, 0.0f, 1.0f };
	int side = WindingOnPlaneSide(w, n, -10.0f);
	OA_ASSERT_INT(side, SIDE_FRONT);
	FreeWinding(w);
}

TEST(windingonplaneside_behind)
{
	winding_t *w = make_square_xy();  /* z = 0 plane */
	/* Plane z = +10 (normal +Z, dist +10): d = 0 - 10 = -10 < -EPS, so the
	 * whole winding is BEHIND the plane. */
	vec3_t n = { 0.0f, 0.0f, 1.0f };
	int side = WindingOnPlaneSide(w, n, 10.0f);
	OA_ASSERT_INT(side, SIDE_BACK);
	FreeWinding(w);
}

TEST(windingonplaneside_crossing)
{
	winding_t *w = make_square_xy();  /* spans y in [-1,1] at z=0 */
	/* Plane y = 0 (normal +Y, dist 0): part of the winding is front (y>0),
	 * part behind (y<0) -> SIDE_CROSS. */
	vec3_t n = { 0.0f, 1.0f, 0.0f };
	int side = WindingOnPlaneSide(w, n, 0.0f);
	OA_ASSERT_INT(side, SIDE_CROSS);
	FreeWinding(w);
}

TEST(chopwindinginplace_splits_off_half)
{
	winding_t *w = make_square_xy();  /* spans x in [-1,1] */
	/* Chop with plane x = 0 (normal +X, dist 0): keeps the front half
	 * (x >= 0), an x in [0,1] strip -> area becomes 2, numpoints 4. */
	vec3_t n = { 1.0f, 0.0f, 0.0f };
	ChopWindingInPlace(&w, n, 0.0f, 0.0f);
	OA_ASSERT_INT(w->numpoints, 4);
	OA_ASSERT_FLOAT(WindingArea(w), 2.0f, 1e-2f);
	vec3_t center;
	WindingCenter(w, center);
	/* center x is now +0.5 */
	OA_ASSERT_FLOAT(center[0], 0.5f, 1e-2f);
	FreeWinding(w);
}

TEST(removecolinearpoints_collapses_midpoint)
{
	/* A 5-point "square" with a redundant point on the RIGHT edge (between
	 * p[1] and p[3]) -> collapses to a 4-point square. */
	winding_t *w = AllocWinding(5);
	w->numpoints = 5;
	w->p[0][0] = -1.0f; w->p[0][1] = -1.0f; w->p[0][2] = 0.0f;
	w->p[1][0] =  1.0f; w->p[1][1] = -1.0f; w->p[1][2] = 0.0f;
	w->p[2][0] =  1.0f; w->p[2][1] =  0.0f; w->p[2][2] = 0.0f; /* redundant, on right edge */
	w->p[3][0] =  1.0f; w->p[3][1] =  1.0f; w->p[3][2] = 0.0f;
	w->p[4][0] = -1.0f; w->p[4][1] =  1.0f; w->p[4][2] = 0.0f;
	RemoveColinearPoints(w);
	OA_ASSERT_INT(w->numpoints, 4);
	FreeWinding(w);
}
