/*
 * tests_qmath5.c - Unit tests for the collision-plane helper BoxOnPlaneSide
 * in qcommon/q_math.c. BoxOnPlaneSide classifies an AABB against a plane and
 * is on the hot path of trace/collision code; its fast axial path and its
 * general signbits path must both be exercised or regressions go unnoticed.
 *
 * Q3 convention used by the engine:
 *   side 1 = box entirely on the POSITIVE side (all corners in front)
 *   side 2 = box entirely on the NEGATIVE side (all corners behind)
 *   side 3 = box straddles the plane (corners on both sides)
 *
 * Build & run (standalone, no SDL/OpenGL needed):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_math5 \
 *       oa_test_run.c test_main.c test_stubs.c tests_qmath5.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_math5
 */
#include "oa_test.h"

#include "q_shared.h"

/* Build an axial plane (type < 3) with the given axis/normal/dist. */
static cplane_t make_axial_plane(int axis, float dist)
{
	cplane_t p;
	Com_Memset(&p, 0, sizeof(p));
	p.normal[0] = p.normal[1] = p.normal[2] = 0.0f;
	p.normal[axis] = 1.0f;
	p.dist = dist;
	p.type = (byte)axis;        /* axial fast path */
	p.signbits = 0;
	return p;
}

TEST(box_on_plane_axial_fully_behind)
{
	/* Plane X=10, box entirely below x=10 (positive side is x>10) -> the
	 * box is entirely on the negative side -> side 2 (behind). */
	cplane_t p = make_axial_plane(0, 10.0f);
	vec3_t mins = { 0, 0, 0 };
	vec3_t maxs = { 5, 5, 5 };
	OA_ASSERT_INT(BoxOnPlaneSide(mins, maxs, &p), 2);
}

TEST(box_on_plane_axial_fully_in_front)
{
	/* Plane X=10, box entirely above x=10 -> side 1 (in front). */
	cplane_t p = make_axial_plane(0, 10.0f);
	vec3_t mins = { 12, 0, 0 };
	vec3_t maxs = { 20, 5, 5 };
	OA_ASSERT_INT(BoxOnPlaneSide(mins, maxs, &p), 1);
}

TEST(box_on_plane_axial_crossing)
{
	/* Plane X=10, box straddles it (mins<10<maxs) -> side 3 (both). */
	cplane_t p = make_axial_plane(0, 10.0f);
	vec3_t mins = { 5, 0, 0 };
	vec3_t maxs = { 15, 5, 5 };
	OA_ASSERT_INT(BoxOnPlaneSide(mins, maxs, &p), 3);
}

TEST(box_on_plane_y_axial_crossing)
{
	cplane_t p = make_axial_plane(1, 0.0f);
	vec3_t mins = { -5, -2, -5 };
	vec3_t maxs = {  5,  2,  5 };
	OA_ASSERT_INT(BoxOnPlaneSide(mins, maxs, &p), 3);
}

TEST(box_on_plane_z_axial_crossing)
{
	cplane_t p = make_axial_plane(2, 3.0f);
	vec3_t mins = { -1, -1, 1 };
	vec3_t maxs = {  1,  1, 5 };
	OA_ASSERT_INT(BoxOnPlaneSide(mins, maxs, &p), 3);
}

TEST(box_on_plane_general_fully_in_front)
{
	/* 45-degree plane: normal (1,1,0)/sqrt2, dist 0. Box fully on the
	 * positive side (both corners project beyond dist) -> side 1. */
	cplane_t p;
	Com_Memset(&p, 0, sizeof(p));
	p.normal[0] = 0.70710678f;
	p.normal[1] = 0.70710678f;
	p.normal[2] = 0.0f;
	p.dist = 0.0f;
	p.type = 3;                 /* non-axial */
	p.signbits = 0;             /* both normal components positive */

	vec3_t mins = { 5, 5, -5 };
	vec3_t maxs = { 10, 10, 5 };
	OA_ASSERT_INT(BoxOnPlaneSide(mins, maxs, &p), 1);
}

TEST(box_on_plane_general_crossing)
{
	/* Same 45-degree plane, box centred so it straddles -> side 3. */
	cplane_t p;
	Com_Memset(&p, 0, sizeof(p));
	p.normal[0] = 0.70710678f;
	p.normal[1] = 0.70710678f;
	p.normal[2] = 0.0f;
	p.dist = 0.0f;
	p.type = 3;
	p.signbits = 0;

	vec3_t mins = { -5, -5, -5 };
	vec3_t maxs = {  5,  5,  5 };
	OA_ASSERT_INT(BoxOnPlaneSide(mins, maxs, &p), 3);
}
