/*
 * tests_qmath7.c - Unit tests for the inline-style vector helpers in
 * qcommon/q_math.c that were still uncovered:
 *
 *   - _DotProduct
 *   - _VectorAdd / _VectorSubtract / _VectorCopy / _VectorScale
 *   - _VectorMA  (vec a + scale*vec b)
 *
 * These are the scalar-component implementations behind the Vector* macros.
 * Pure functions, link standalone with q_shared.c + q_math.c + -lm.
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qmath7.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_all
 */

#include "oa_test.h"

#include "q_shared.h"

TEST(dotproduct_basic)
{
	vec3_t a = { 1.0f, 2.0f, 3.0f };
	vec3_t b = { 4.0f, -5.0f, 6.0f };
	vec3_t expect = { 11.0f, 22.0f, 33.0f };
	/* 1*4 + 2*-5 + 3*6 = 4 - 10 + 18 = 12 */
	OA_ASSERT_FLOAT(_DotProduct(a, b), 12.0f, 1e-5f);
}

TEST(dotproduct_orthogonal_is_zero)
{
	vec3_t x = { 1.0f, 0.0f, 0.0f };
	vec3_t y = { 0.0f, 1.0f, 0.0f };
	vec3_t z = { 0.0f, 0.0f, 1.0f };
	OA_ASSERT_FLOAT(_DotProduct(x, y), 0.0f, 1e-6f);
	OA_ASSERT_FLOAT(_DotProduct(x, z), 0.0f, 1e-6f);
}

TEST(vectoradd_basic)
{
	vec3_t a = { 1.0f, 2.0f, 3.0f };
	vec3_t b = { 10.0f, 20.0f, 30.0f };
	vec3_t out, expect = { 11.0f, 22.0f, 33.0f };
	_VectorAdd(a, b, out);
	OA_ASSERT_VEC3(out, expect, 1e-6f);
}

TEST(vectorsubtract_basic)
{
	vec3_t a = { 10.0f, 20.0f, 30.0f };
	vec3_t b = { 1.0f, 2.0f, 3.0f };
	vec3_t out, expect = { 9.0f, 18.0f, 27.0f };
	_VectorSubtract(a, b, out);
	OA_ASSERT_VEC3(out, expect, 1e-6f);
}

TEST(vectorcopy_basic)
{
	vec3_t in = { 5.0f, -6.0f, 7.0f };
	vec3_t out = { 0.0f, 0.0f, 0.0f };
	_VectorCopy(in, out);
	OA_ASSERT_VEC3(out, in, 1e-6f);
}

TEST(vectorscale_basic)
{
	vec3_t in = { 1.0f, -2.0f, 3.0f };
	vec3_t out, expect = { 2.5f, -5.0f, 7.5f };
	_VectorScale(in, 2.5f, out);
	OA_ASSERT_VEC3(out, expect, 1e-6f);
}

TEST(vectorma_basic)
{
	/* vecc = veca + scale*vec b */
	vec3_t a = { 1.0f, 1.0f, 1.0f };
	vec3_t b = { 2.0f, 3.0f, 4.0f };
	vec3_t out, expect = { 5.0f, 7.0f, 9.0f };
	_VectorMA(a, 2.0f, b, out);
	OA_ASSERT_VEC3(out, expect, 1e-6f);
}

TEST(vectorma_zero_scale)
{
	vec3_t a = { 1.0f, 1.0f, 1.0f };
	vec3_t b = { 2.0f, 3.0f, 4.0f };
	vec3_t out;
	_VectorMA(a, 0.0f, b, out);
	OA_ASSERT_VEC3(out, a, 1e-6f);
}
