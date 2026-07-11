/*
 * tests_qshared2.c - Unit tests for the previously uncovered standalone
 * string/compare/endian helpers in qcommon/q_shared.c:
 *
 *   - Q_strncmp      (bounded compare; embedded-NUL / short-string handling)
 *   - COM_CompareExtension (case-insensitive tail match)
 *   - ShortNoSwap / LongNoSwap / Long64NoSwap / FloatNoSwap (host-order
 *     pass-through, used on the build's native endianness)
 *
 * These are pure functions (no engine state, no SDL/GL) and link standalone
 * with q_shared.c. The *NoSwap prototypes are commented out in q_shared.h
 * (legacy), so they are forward-declared here (gcc 14 makes implicit
 * declarations hard errors otherwise).
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared2.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_all
 */

#include "oa_test.h"

#include "q_shared.h"

/* Forward declarations (prototypes are commented out in q_shared.h). */
short  ShortNoSwap(short l);
int    LongNoSwap(int l);
qint64 Long64NoSwap(qint64 ll);
float  FloatNoSwap(const float *f);

/* ------------------------------------------------------------------ */
/* Q_strncmp                                                          */
/* ------------------------------------------------------------------ */

TEST(qstrncmp_equal_full)
{
	OA_ASSERT_INT(Q_strncmp("hello", "hello", 5), 0);
	OA_ASSERT_INT(Q_strncmp("hello", "hello", 10), 0);
}

TEST(qstrncmp_count_limits_compare)
{
	/* Only the first `n` chars are compared, so a longer mismatch is ignored. */
	OA_ASSERT_INT(Q_strncmp("abcde", "abcZZ", 3), 0);   /* first 3 equal */
	OA_ASSERT_INT(Q_strncmp("abc", "abcZZ", 3), 0);     /* equal within n */
	/* first difference within n is detected */
	OA_ASSERT(Q_strncmp("abc", "abd", 3) != 0);
	OA_ASSERT_INT(Q_strncmp("abc", "abd", 2), 0);       /* only first 2 differ? no: 2 equal */
}

TEST(qstrncmp_returns_sign)
{
	/* c1 < c2 -> negative; c1 > c2 -> positive */
	OA_ASSERT(Q_strncmp("abc", "abd", 3) < 0);
	OA_ASSERT(Q_strncmp("abd", "abc", 3) > 0);
}

TEST(qstrncmp_embedded_nul)
{
	/* Comparison stops at the first NUL in either string (before n). */
	OA_ASSERT_INT(Q_strncmp("ab\0cd", "ab\0xy", 10), 0);
	/* but a real difference before the NUL is still found */
	OA_ASSERT(Q_strncmp("ab\0cd", "ax\0cd", 10) != 0);
}

TEST(qstrncmp_zero_count)
{
	/* n == 0 means "equal until end point" -> 0 */
	OA_ASSERT_INT(Q_strncmp("anything", "different", 0), 0);
}

/* ------------------------------------------------------------------ */
/* COM_CompareExtension                                               */
/* ------------------------------------------------------------------ */

TEST(compareextension_match_case_insensitive)
{
	/* extension tail match, case-insensitive */
	OA_ASSERT_INT(COM_CompareExtension("FOO.PK3", ".pk3"), qtrue);
	OA_ASSERT_INT(COM_CompareExtension("foo.pk3", ".PK3"), qtrue);
	OA_ASSERT_INT(COM_CompareExtension("foo.pk3", ".pk3"), qtrue);
}

TEST(compareextension_sub_extension)
{
	/* ".pk3" matches the tail of "bar.pk3" even though the string is longer */
	OA_ASSERT_INT(COM_CompareExtension("bar.pk3", ".pk3"), qtrue);
	/* ".pk" must NOT match "bar.pk3" (the actual tail is ".pk3") */
	OA_ASSERT_INT(COM_CompareExtension("bar.pk3", ".pk"), qfalse);
}

TEST(compareextension_no_match)
{
	OA_ASSERT_INT(COM_CompareExtension("foo.txt", ".pk3"), qfalse);
	OA_ASSERT_INT(COM_CompareExtension("foo", ".pk3"), qfalse);
}

/* ------------------------------------------------------------------ */
/* *NoSwap (host-order pass-through)                                  */
/* ------------------------------------------------------------------ */

TEST(shortnoswap_identity)
{
	OA_ASSERT_INT(ShortNoSwap((short)0x1234), (short)0x1234);
	OA_ASSERT_INT(ShortNoSwap((short)-1), (short)-1);
}

TEST(longnoswap_identity)
{
	OA_ASSERT_INT(LongNoSwap(0x12345678), 0x12345678);
	OA_ASSERT_INT(LongNoSwap((int)0xDEADBEEF), (int)0xDEADBEEF);
}

TEST(long64noswap_identity)
{
	qint64 in, out;
	in.b0 = 0x11; in.b1 = 0x22; in.b2 = 0x33; in.b3 = 0x44;
	in.b4 = 0x55; in.b5 = 0x66; in.b6 = 0x77; in.b7 = 0x88;
	out = Long64NoSwap(in);
	OA_ASSERT_INT(out.b0, 0x11); OA_ASSERT_INT(out.b1, 0x22);
	OA_ASSERT_INT(out.b2, 0x33); OA_ASSERT_INT(out.b3, 0x44);
	OA_ASSERT_INT(out.b4, 0x55); OA_ASSERT_INT(out.b5, 0x66);
	OA_ASSERT_INT(out.b6, 0x77); OA_ASSERT_INT(out.b7, 0x88);
}

TEST(floatnoswap_identity)
{
	float f = 3.14159265f;
	OA_ASSERT_FLOAT(FloatNoSwap(&f), f, 1e-9f);
	float g = -2.5f;
	OA_ASSERT_FLOAT(FloatNoSwap(&g), g, 1e-9f);
}
