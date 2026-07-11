/*
 * tests_qmsg7.c - Player-state delta serialization round-trip tests for
 * MSG_WriteDeltaPlayerstate / MSG_ReadDeltaPlayerstate in qcommon/msg.c.
 * These serialize the per-client playerState (movement, stats, ammo,
 * powerups) every frame; a writer/reader mismatch desyncs the local player's
 * view, so an exact round-trip must be locked down. Unlike entityState,
 * these functions accept from=NULL (they substitute an internal zeroed
 * dummy), which makes a "full" write straightforward.
 *
 * Build & run (standalone, no SDL/OpenGL needed):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_msg7 \
 *       oa_test_run.c test_main.c test_stubs.c tests_qmsg7.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_msg7
 */
#include "oa_test.h"

#include "q_shared.h"
#include "qcommon.h"

static void fill_ps(playerState_t *s)
{
	Com_Memset(s, 0, sizeof(*s));
	s->commandTime = 1000;
	s->pm_type = 3;
	s->bobCycle = 7;
	s->pm_flags = 0x20;
	s->pm_time = 50;
	s->origin[0] = 1.5f; s->origin[1] = -2.5f; s->origin[2] = 3.5f;
	s->velocity[0] = 10.0f; s->velocity[1] = -20.0f; s->velocity[2] = 30.0f;
	s->weaponTime = 200;
	s->gravity = 800;
	s->speed = 320;
	s->delta_angles[0] = 100; s->delta_angles[1] = 200; s->delta_angles[2] = 300;
	s->groundEntityNum = 9;
	s->legsTimer = 11;
	s->legsAnim = 12;
	s->torsoTimer = 13;
	s->torsoAnim = 14;
	s->movementDir = 4;
	s->eFlags = 0x40;
	s->eventSequence = 2;
	s->events[0] = 50; s->events[1] = 51;
	s->eventParms[0] = 60; s->eventParms[1] = 61;
	s->externalEvent = 70; s->externalEventParm = 71;
	s->viewangles[0] = 0.1f; s->viewangles[1] = 0.2f;
	s->viewheight = 26;
	s->damageEvent = 1; s->damageYaw = 180; s->damagePitch = 45; s->damageCount = 5;
	s->generic1 = 33;
	/* populate the communicated arrays */
	for (int i = 0; i < MAX_STATS; i++)      s->stats[i] = i * 2;
	for (int i = 0; i < MAX_PERSISTANT; i++) s->persistant[i] = i * 3;
	for (int i = 0; i < MAX_POWERUPS; i++)   s->powerups[i] = i * 4;
	for (int i = 0; i < MAX_WEAPONS; i++)    s->ammo[i] = i * 5;
}

/* Compare every 32-bit word of two player states for equality. */
static void assert_ps_equal(const playerState_t *a, const playerState_t *b)
{
	const int *aw = (const int *)a;
	const int *bw = (const int *)b;
	int n = (int)(sizeof(*a) / 4);
	for (int i = 0; i < n; i++) {
		OA_ASSERT_INT(aw[i], bw[i]);
	}
}

TEST(ps_delta_full_roundtrip_from_null)
{
	/* Full write using a NULL baseline (engine substitutes a zeroed
	 * dummy, so every field is transmitted). Read back from NULL and
	 * confirm the whole state survived. */
	playerState_t in, out;
	fill_ps(&in);

	byte data[2048];
	msg_t buf;
	MSG_Init(&buf, data, sizeof(data));
	MSG_Bitstream(&buf);

	MSG_WriteDeltaPlayerstate(&buf, NULL, &in);

	MSG_BeginReading(&buf);
	Com_Memset(&out, 0, sizeof(out));
	MSG_ReadDeltaPlayerstate(&buf, NULL, &out);

	assert_ps_equal(&in, &out);
}

TEST(ps_delta_partial_from_baseline)
{
	/* Write a delta from a baseline; only changed fields are sent, but the
	 * reader reconstructs the FULL state from the baseline. This exercises
	 * both the scalar delta and the array-bitmask paths. */
	playerState_t base, changed, out;
	fill_ps(&base);
	changed = base;
	changed.origin[0] = 99.0f;          /* scalar change (serialized) */
	changed.stats[3] = 777;             /* array change (stats) */
	changed.ammo[5] = 888;              /* array change (ammo) */
	changed.powerups[2] = 999;          /* array change (powerups) */

	byte data[2048];
	msg_t buf;
	MSG_Init(&buf, data, sizeof(data));
	MSG_Bitstream(&buf);

	MSG_WriteDeltaPlayerstate(&buf, &base, &changed);

	MSG_BeginReading(&buf);
	Com_Memset(&out, 0, sizeof(out));
	MSG_ReadDeltaPlayerstate(&buf, &base, &out);

	/* unchanged baseline fields survive, changed ones come through.
	 * Note: only fields in playerStateFields (plus the four arrays) are
	 * transmitted; clientNum/weapon/viewangles[2]/grapplePoint are NOT on
	 * the wire, so we assert only on serialized fields. */
	OA_ASSERT_FLOAT(out.origin[0], 99.0f, 1e-3f);
	OA_ASSERT_INT(out.weaponTime, 200);
	OA_ASSERT_INT(out.stats[3], 777);
	OA_ASSERT_INT(out.ammo[5], 888);
	OA_ASSERT_INT(out.powerups[2], 999);
	OA_ASSERT_INT(out.pm_type, base.pm_type);
	OA_ASSERT_INT(out.eventSequence, base.eventSequence);
	assert_ps_equal(&changed, &out);
}

TEST(ps_delta_no_change_is_identity)
{
	/* Writing a playerState against an identical baseline produces a
	 * minimal record; reading it back yields exactly the baseline state
	 * (lc == 0, no array bits set). */
	playerState_t base, out;
	fill_ps(&base);

	byte data[2048];
	msg_t buf;
	MSG_Init(&buf, data, sizeof(data));
	MSG_Bitstream(&buf);

	MSG_WriteDeltaPlayerstate(&buf, &base, &base);

	MSG_BeginReading(&buf);
	Com_Memset(&out, 0, sizeof(out));
	MSG_ReadDeltaPlayerstate(&buf, &base, &out);

	assert_ps_equal(&base, &out);
}
