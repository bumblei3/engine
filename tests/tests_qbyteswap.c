/*
 * tests_qbyteswap.c - Unit tests for the byte-swap / endianness helpers in
 * qcommon/q_shared.c (ShortSwap, LongSwap, Long64Swap, FloatSwap,
 * CopyShortSwap, CopyLongSwap).
 *
 * These are the portable serialization primitives used for network messages
 * and on-disk formats, so they must round-trip and reverse correctly on any
 * host byte order. They are pure functions (no engine state), so they link
 * standalone.
 *
 * Build & run (standalone, no SDL/OpenGL needed):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_byteswap \
 *       oa_test_run.c test_main.c test_stubs.c tests_qbyteswap.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_byteswap
 */
#include "oa_test.h"

#include "q_shared.h"

/* The byte-swap primitives are defined in q_shared.c but their prototypes are
 * commented out in q_shared.h (legacy). Declare them here so the tests link
 * without relying on the (absent) header declarations. */
short  ShortSwap(short l);
int    LongSwap(int l);
qint64 Long64Swap(qint64 ll);
float  FloatSwap(const float *f);
void   CopyShortSwap(void *dest, void *src);
void   CopyLongSwap(void *dest, void *src);

TEST(shortswap_reverses_bytes)
{
	short in = 0x1234;          /* b1=0x34 (LSB), b2=0x12 (MSB) */
	short out = ShortSwap(in);
	/* swapped: low byte becomes high, high becomes low */
	OA_ASSERT_INT(((out >> 8) & 0xFF), 0x34);
	OA_ASSERT_INT((out & 0xFF), 0x12);
}

TEST(shortswap_involution)
{
	/* applying the swap twice returns the original value */
	short in = 0xABCD;
	OA_ASSERT_INT(ShortSwap(ShortSwap(in)), in);
}

TEST(shortswap_noop_on_symmetric_value)
{
	/* a value whose bytes are equal is unchanged by swapping */
	OA_ASSERT_INT(ShortSwap((short)0x2222), (short)0x2222);
}

TEST(longswap_reverses_bytes)
{
	int in = 0x01020304;
	int out = LongSwap(in);
	/* bytes 1,2,3,4 (little-endian order in memory) become 4,3,2,1 */
	OA_ASSERT_INT(((out >> 24) & 0xFF), 0x04);
	OA_ASSERT_INT(((out >> 16) & 0xFF), 0x03);
	OA_ASSERT_INT(((out >> 8) & 0xFF), 0x02);
	OA_ASSERT_INT((out & 0xFF), 0x01);
}

TEST(longswap_involution)
{
	int in = 0xDEADBEEF;
	OA_ASSERT_INT(LongSwap(LongSwap(in)), in);
}

TEST(longswap_noop_on_palindrome)
{
	/* A value whose byte pattern is front-to-back symmetric reverses to
	 * itself: 0x00112233 -> bytes 00 11 22 33 -> 33 22 11 00 (not symmetric),
	 * so instead use 0x00112200 which is NOT symmetric either. Use a truly
	 * symmetric byte pattern: each byte equals its mirror (0x00 0x11 0x11 0x00). */
	OA_ASSERT_INT(LongSwap(0x00111100), 0x00111100);
}

TEST(copyshortswap_reverses_in_place)
{
	byte src[2] = { 0x34, 0x12 };
	byte dst[2] = { 0, 0 };
	CopyShortSwap(dst, src);
	OA_ASSERT_INT(dst[0], 0x12);
	OA_ASSERT_INT(dst[1], 0x34);
}

TEST(copylongswap_reverses_in_place)
{
	byte src[4] = { 0x01, 0x02, 0x03, 0x04 };
	byte dst[4] = { 0, 0, 0, 0 };
	CopyLongSwap(dst, src);
	OA_ASSERT_INT(dst[0], 0x04);
	OA_ASSERT_INT(dst[1], 0x03);
	OA_ASSERT_INT(dst[2], 0x02);
	OA_ASSERT_INT(dst[3], 0x01);
}

TEST(copyshort_alias_is_not_supported)
{
	/* CopyShortSwap reads src[1] into dst[0] BEFORE writing dst[1], so with
	 * overlapping src==dst the second read picks up the already-overwritten
	 * src[0]. The engine always passes separate buffers; assert the actual
	 * behaviour so a future change is caught. Result: both bytes become the
	 * original low byte (0xBB) after the swap, because src[0] is clobbered
	 * first. */
	byte buf[2] = { 0xAA, 0xBB };
	CopyShortSwap(buf, buf);
	OA_ASSERT_INT(buf[0], 0xBB);
	OA_ASSERT_INT(buf[1], 0xBB);
}

TEST(long64swap_reverses_all_eight_bytes)
{
	qint64 in;
	in.b0 = 0x00; in.b1 = 0x11; in.b2 = 0x22; in.b3 = 0x33;
	in.b4 = 0x44; in.b5 = 0x55; in.b6 = 0x66; in.b7 = 0x77;

	qint64 out = Long64Swap(in);

	OA_ASSERT_INT(out.b0, 0x77);
	OA_ASSERT_INT(out.b1, 0x66);
	OA_ASSERT_INT(out.b2, 0x55);
	OA_ASSERT_INT(out.b3, 0x44);
	OA_ASSERT_INT(out.b4, 0x33);
	OA_ASSERT_INT(out.b5, 0x22);
	OA_ASSERT_INT(out.b6, 0x11);
	OA_ASSERT_INT(out.b7, 0x00);
}

TEST(long64swap_involution)
{
	qint64 in;
	in.b0 = 0x10; in.b1 = 0x20; in.b2 = 0x30; in.b3 = 0x40;
	in.b4 = 0x50; in.b5 = 0x60; in.b6 = 0x70; in.b7 = 0x80;

	qint64 round = Long64Swap(Long64Swap(in));
	OA_ASSERT_INT(round.b0, in.b0);
	OA_ASSERT_INT(round.b1, in.b1);
	OA_ASSERT_INT(round.b2, in.b2);
	OA_ASSERT_INT(round.b3, in.b3);
	OA_ASSERT_INT(round.b4, in.b4);
	OA_ASSERT_INT(round.b5, in.b5);
	OA_ASSERT_INT(round.b6, in.b6);
	OA_ASSERT_INT(round.b7, in.b7);
}

TEST(floatswap_roundtrips_value)
{
	/* FloatSwap must reverse the bytes but preserve the numeric value
	 * once swapped back (it is a reordering, not a conversion). */
	float v = 3.14159265358979f;
	float swapped = FloatSwap(&v);
	float back = FloatSwap(&swapped);
	OA_ASSERT_FLOAT(back, v, 1e-9);
}

TEST(floatswap_changes_byte_order)
{
	float v = 1.0f;
	byte *pb = (byte *)&v;
	float swapped = FloatSwap(&v);
	byte *ps = (byte *)&swapped;
	/* little-endian host: byte 0 of 1.0f is 0x00, MSB is 0x3F. After swap
	 * the LSB of the swapped value must equal the original MSB. */
	OA_ASSERT_INT(ps[0], pb[3]);
	OA_ASSERT_INT(ps[3], pb[0]);
}
