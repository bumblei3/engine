/*
 * tests_msg3.c - Unit tests for additional network-message helpers in
 * qcommon/msg.c that were still uncovered:
 *
 *   - MSG_WriteAngle        (angle -> 1 byte: (int)(f*256/360) & 255)
 *   - MSG_WriteDeltaUsercmdKey / MSG_ReadDeltaUsercmdKey
 *                            (per-key usercmd delta codec: serverTime +
 *                             angles/move/buttons/weapon, with a "no change"
 *                             short form and a small/large serverTime form)
 *
 * These run in the main test_all binary (msg.c is already in ENG_SRC) and
 * are genuine serialize/deserialize wire-format tests. The usercmd_t struct
 * comes from qcommon.h.
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_msg3.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_all
 */

#include "oa_test.h"

#include "q_shared.h"
#include "qcommon.h"

/* MSG_WriteAngle is defined in msg.c but not declared in qcommon.h (like the
 * byte-swap primitives), so forward-declare it for the standalone build.
 * MSG_WriteDeltaUsercmdKey/ReadDeltaUsercmdKey ARE declared in qcommon.h. */
void MSG_WriteAngle(msg_t *sb, float f);

static void fill_usercmd(usercmd_t *u, int st, int a0, int a1, int a2,
                         int fwd, int right, int up, int btn, int wpn)
{
	Com_Memset(u, 0, sizeof(*u));
	u->serverTime = st;
	u->angles[0] = a0; u->angles[1] = a1; u->angles[2] = a2;
	u->forwardmove = fwd; u->rightmove = right; u->upmove = up;
	u->buttons = btn; u->weapon = wpn;
}

TEST(msgwriteangle_byte)
{
	/* MSG_WriteAngle encodes (int)(f*256/360) & 255. In OOB (raw) mode the
	 * byte is written verbatim. For 90 deg -> 64. */
	msg_t m; byte data[64];
	Com_Memset(&m, 0, sizeof(m));
	m.data = data; m.maxsize = sizeof(data);
	MSG_InitOOB(&m, data, sizeof(data));
	MSG_WriteAngle(&m, 90.0f);
	OA_ASSERT_INT(m.cursize, 1);
	OA_ASSERT_INT(data[0], 64);
}

TEST(msgwriteangle_zero_and_full)
{
	msg_t m; byte data[64];
	Com_Memset(&m, 0, sizeof(m));
	m.data = data; m.maxsize = sizeof(data);
	MSG_InitOOB(&m, data, sizeof(data));
	MSG_WriteAngle(&m, 0.0f);
	OA_ASSERT_INT(data[0], 0);

	Com_Memset(&m, 0, sizeof(m));
	m.data = data; m.maxsize = sizeof(data);
	MSG_InitOOB(&m, data, sizeof(data));
	MSG_WriteAngle(&m, 360.0f);
	/* 360*256/360 = 256, &255 = 0 */
	OA_ASSERT_INT(data[0], 0);
}

TEST(deltausercmdkey_small_servertime_roundtrip)
{
	/* serverTime delta < 256 -> small 8-bit form. */
	usercmd_t from, to, out;
	fill_usercmd(&from, 1000, 10, 20, 30, 5, 6, 7, 1, 2);
	fill_usercmd(&to,   1050, 11, 21, 31, 5, 6, 7, 1, 2); /* only st + angles differ */

	msg_t m; byte data[256];
	Com_Memset(&m, 0, sizeof(m));
	m.data = data; m.maxsize = sizeof(data);

	MSG_WriteDeltaUsercmdKey(&m, 0, &from, &to);
	OA_ASSERT(m.cursize > 0);

	MSG_BeginReading(&m);
	Com_Memset(&out, 0, sizeof(out));
	MSG_ReadDeltaUsercmdKey(&m, 0, &from, &out);

	OA_ASSERT_INT(out.serverTime, 1050);
	OA_ASSERT_INT(out.angles[0], 11);
	OA_ASSERT_INT(out.angles[1], 21);
	OA_ASSERT_INT(out.angles[2], 31);
	/* unchanged fields come from `from` */
	OA_ASSERT_INT(out.forwardmove, 5);
	OA_ASSERT_INT(out.buttons, 1);
	OA_ASSERT_INT(out.weapon, 2);
}

TEST(deltausercmdkey_large_servertime_roundtrip)
{
	/* serverTime delta >= 256 -> full 32-bit form. */
	usercmd_t from, to, out;
	fill_usercmd(&from, 1000, 0, 0, 0, 0, 0, 0, 0, 0);
	fill_usercmd(&to,   2000, 0, 0, 0, 0, 0, 0, 0, 0);

	msg_t m; byte data[256];
	Com_Memset(&m, 0, sizeof(m));
	m.data = data; m.maxsize = sizeof(data);

	MSG_WriteDeltaUsercmdKey(&m, 0, &from, &to);
	MSG_BeginReading(&m);
	Com_Memset(&out, 0, sizeof(out));
	MSG_ReadDeltaUsercmdKey(&m, 0, &from, &out);
	OA_ASSERT_INT(out.serverTime, 2000);
}

TEST(deltausercmdkey_no_change_short_form)
{
	/* from == to -> "no change" bit, reader copies `from` verbatim. */
	usercmd_t from, to, out;
	fill_usercmd(&from, 1234, 40, 50, 60, 9, 8, 7, 3, 4);
	Com_Memcpy(&to, &from, sizeof(to));  /* identical */

	msg_t m; byte data[256];
	Com_Memset(&m, 0, sizeof(m));
	m.data = data; m.maxsize = sizeof(data);

	MSG_WriteDeltaUsercmdKey(&m, 0, &from, &to);
	MSG_BeginReading(&m);
	Com_Memset(&out, 0, sizeof(out));
	MSG_ReadDeltaUsercmdKey(&m, 0, &from, &out);

	/* every field reconstructed from `from` */
	OA_ASSERT_INT(out.serverTime, 1234);
	OA_ASSERT_INT(out.angles[0], 40);
	OA_ASSERT_INT(out.forwardmove, 9);
	OA_ASSERT_INT(out.weapon, 4);
}
