/*
 * tests_qmsg5.c - Bit-exact message round-trip tests for the Huffman bit
 * stream in qcommon/msg.c (MSG_WriteBits / MSG_ReadBits). The bit stream is
 * the wire format for entity-state deltas; mismatched bit widths or sign
 * handling between writer and reader are a classic source of network
 * desync, so we assert EXACT recovery across mixed widths and signs.
 *
 * NOTE: a 0-bit write and out-of-range bit counts are treated as
 * Com_Error(ERR_DROP) by the engine, so those error paths live in the
 * nofail overflow binary, not here (this suite uses the aborting stub).
 *
 * Build & run (standalone, no SDL/OpenGL needed):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_msg5 \
 *       oa_test_run.c test_main.c test_stubs.c tests_qmsg5.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_msg5
 */
#include "oa_test.h"

#include "q_shared.h"
#include "qcommon.h"

/* Huffman mode (oob = qfalse): bits are entropy-coded. Round-trip must
 * recover the exact integer for any supported width and sign. */
TEST(msg_bits_roundtrip_unsigned_widths)
{
	byte data[64];
	msg_t buf;
	MSG_Init(&buf, data, sizeof(data));
	MSG_Bitstream(&buf);

	/* write a sequence of unsigned values across mixed widths */
	MSG_WriteBits(&buf, 0, 1);
	MSG_WriteBits(&buf, 1, 1);
	MSG_WriteBits(&buf, 5, 4);
	MSG_WriteBits(&buf, 255, 8);
	MSG_WriteBits(&buf, 1023, 10);
	MSG_WriteBits(&buf, 70000, 17);
	MSG_WriteBits(&buf, 0xFFFFFFFF, 32);

	MSG_BeginReading(&buf);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 1), 0);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 1), 1);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 4), 5);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 8), 255);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 10), 1023);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 17), 70000);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 32), (int)0xFFFFFFFF);
}

TEST(msg_bits_roundtrip_signed_widths)
{
	/* Signed interpretation uses NEGATIVE bit counts. Tested at 8 and 16
	 * bits, which round-trip cleanly through the Huffman path. NOTE: very
	 * wide signed widths (>= ~30 bits) do NOT reliably round-trip here
	 * because the engine's sign-extension uses `1 << bits`, which is
	 * undefined behaviour at bit 31 on 32-bit ints; those widths are
	 * intentionally excluded. */
	byte data[64];
	msg_t buf;
	MSG_Init(&buf, data, sizeof(data));
	MSG_Bitstream(&buf);

	MSG_WriteBits(&buf, -128, -8);
	MSG_WriteBits(&buf,  127, -8);
	MSG_WriteBits(&buf, -30000, -16);
	MSG_WriteBits(&buf,  30000, -16);

	MSG_BeginReading(&buf);
	OA_ASSERT_INT(MSG_ReadBits(&buf, -8), -128);
	OA_ASSERT_INT(MSG_ReadBits(&buf, -8),  127);
	OA_ASSERT_INT(MSG_ReadBits(&buf, -16), -30000);
	OA_ASSERT_INT(MSG_ReadBits(&buf, -16),  30000);
}

TEST(msg_bits_interleaved_alignment)
{
	/* Interleave odd-width unsigned writes (3, 5, 7 bits) then read back in
	 * the same order. This exercises bit-packing across byte boundaries. */
	byte data[64];
	msg_t buf;
	MSG_Init(&buf, data, sizeof(data));
	MSG_Bitstream(&buf);

	MSG_WriteBits(&buf, 0x5, 3);    /* 101 */
	MSG_WriteBits(&buf, 0x11, 5);   /* 10001 */
	MSG_WriteBits(&buf, 0x55, 7);   /* 1010101 */
	MSG_WriteBits(&buf, 0x7F, 7);
	MSG_WriteBits(&buf, 0x6, 3);

	MSG_BeginReading(&buf);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 3), 0x5);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 5), 0x11);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 7), 0x55);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 7), 0x7F);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 3), 0x6);
}

TEST(msg_oob_writebits_raw_bytes)
{
	/* In OOB mode, WriteBits writes raw little-endian bytes (no Huffman). */
	byte data[64];
	msg_t buf;
	MSG_InitOOB(&buf, data, sizeof(data));

	MSG_WriteBits(&buf, 0x1234, 16);
	OA_ASSERT_INT(buf.cursize, 2);
	/* little-endian: low byte first */
	OA_ASSERT_INT(data[0], 0x34);
	OA_ASSERT_INT(data[1], 0x12);

	MSG_BeginReadingOOB(&buf);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 16), 0x1234);
}

TEST(msg_oob_writebits_32)
{
	byte data[64];
	msg_t buf;
	MSG_InitOOB(&buf, data, sizeof(data));

	MSG_WriteBits(&buf, 0xDEADBEEF, 32);
	OA_ASSERT_INT(buf.cursize, 4);
	MSG_BeginReadingOOB(&buf);
	OA_ASSERT_INT(MSG_ReadBits(&buf, 32), (int)0xDEADBEEF);
}

TEST(msg_writebits_tiny_buffer_sets_overflow)
{
	/* A buffer too small to hold the write must set overflowed and not
	 * grow cursize past maxsize. No Com_Error is raised for overflow. */
	byte data[4];
	msg_t buf;
	MSG_Init(&buf, data, sizeof(data));
	MSG_Bitstream(&buf);

	/* write enough to exceed 4 bytes */
	MSG_WriteBits(&buf, 0x01020304, 32);
	MSG_WriteBits(&buf, 0x05060708, 32);

	OA_ASSERT(buf.overflowed == qtrue);
	OA_ASSERT_INT(buf.cursize, sizeof(data));
}
