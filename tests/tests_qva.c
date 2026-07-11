/*
 * tests_qva.c - Tests for va() (varargs string formatting) and Com_Clamp in
 * qcommon/q_shared.c. va() uses a ping-pong pair of static 32k buffers keyed
 * by a rolling index, so consecutive (and nested) calls must not clobber
 * each other's result -- a classic reentrancy bug surface. Com_Clamp is the
 * trivial but widely-used saturating clamp.
 *
 * Build & run (standalone, no SDL/OpenGL needed):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_va \
 *       oa_test_run.c test_main.c test_stubs.c tests_qva.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_va
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(va_basic_format)
{
	const char *r = va("value=%d", 42);
	OA_ASSERT_STR(r, "value=42");
}

TEST(va_multiple_args)
{
	const char *r = va("%s:%d:%s", "host", 27960, "name");
	OA_ASSERT_STR(r, "host:27960:name");
}

TEST(va_consecutive_calls_independent_buffers)
{
	/* va() ping-pongs between two static buffers, so two consecutive calls
	 * must return pointers to distinct buffers holding their own text
	 * (the second call must NOT overwrite the first's result). */
	const char *a = va("first-%d", 1);
	const char *b = va("second-%d", 2);
	/* both strings must still be intact and distinct */
	OA_ASSERT_STR(a, "first-1");
	OA_ASSERT_STR(b, "second-2");
	OA_ASSERT(a != b);
}

TEST(va_nested_call_does_not_clobber_outer)
{
	/* A nested va() call (inside the format arguments of an outer one)
	 * must not destroy the outer result, because the rolling index moves
	 * the nested result to the OTHER buffer. */
	const char *outer = va("outer=%s", va("inner-%d", 7));
	/* the outer string must contain the fully-formatted inner value */
	OA_ASSERT_STR(outer, "outer=inner-7");
}

TEST(com_clamp_saturates_low)
{
	OA_ASSERT_FLOAT(Com_Clamp(0.0f, 10.0f, -5.0f), 0.0f, 1e-6f);
}

TEST(com_clamp_saturates_high)
{
	OA_ASSERT_FLOAT(Com_Clamp(0.0f, 10.0f, 50.0f), 10.0f, 1e-6f);
}

TEST(com_clamp_passes_through_in_range)
{
	OA_ASSERT_FLOAT(Com_Clamp(0.0f, 10.0f, 5.0f), 5.0f, 1e-6f);
}

TEST(com_clamp_equals_min)
{
	OA_ASSERT_FLOAT(Com_Clamp(3.0f, 8.0f, 3.0f), 3.0f, 1e-6f);
}
