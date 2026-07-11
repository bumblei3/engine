/*
 * tests_qmsg6.c - Entity-state delta serialization round-trip tests for
 * MSG_WriteDeltaEntity / MSG_ReadDeltaEntity in qcommon/msg.c. These are the
 * wire-format functions for the per-entity update messages; a mismatch
 * between writer and reader desyncs every entity a client renders, so an
 * exact round-trip must be locked down.
 *
 * NOTE on baselines: the engine's MSG_WriteDeltaEntity dereferences `from`
 * inside its change-detection loop, so `from` must be a valid (non-NULL)
 * baseline whenever `to` is non-NULL. A "full" write is achieved by passing
 * a baseline that differs in every field (here: an all-zero baseline), which
 * forces every field to be transmitted as a change. Passing from=NULL with
 * to!=NULL dereferences the NULL page and is NOT supported by this engine.
 *
 * The engine's packet path reads the entity NUMBER before calling
 * MSG_ReadDeltaEntity (which takes `number` as a parameter and does NOT
 * consume it from the stream). We replicate that: after writing, we read the
 * GENTITYNUM_BITS number back, then call ReadDeltaEntity with it.
 *
 * Build & run (standalone, no SDL/OpenGL needed):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_msg6 \
 *       oa_test_run.c test_main.c test_stubs.c tests_qmsg6.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_msg6
 */
#include "oa_test.h"

#include "q_shared.h"
#include "qcommon.h"

static void fill_state(entityState_t *s, int number)
{
	Com_Memset(s, 0, sizeof(*s));
	s->number = number;
	s->eType = 4;
	s->eFlags = 0x10;
	s->pos.trType = 1;
	s->pos.trTime = 500;
	s->pos.trDuration = 100;
	s->pos.trBase[0] = 1.0f; s->pos.trBase[1] = 2.0f; s->pos.trBase[2] = 3.0f;
	s->pos.trDelta[0] = 4.0f; s->pos.trDelta[1] = 5.0f; s->pos.trDelta[2] = 6.0f;
	s->apos.trType = 2;
	s->time = 1234;
	s->time2 = 5678;
	s->origin[0] = -10.5f; s->origin[1] = 20.25f; s->origin[2] = 30.0f;
	s->origin2[0] = 1.5f; s->origin2[1] = 2.5f; s->origin2[2] = 3.5f;
	s->angles[0] = 0.1f; s->angles[1] = 0.2f; s->angles[2] = 0.3f;
	s->angles2[0] = 0.4f; s->angles2[1] = 0.5f; s->angles2[2] = 0.6f;
	s->otherEntityNum = 7;
	s->otherEntityNum2 = 8;
	s->groundEntityNum = 9;
	s->constantLight = 0x11223344;
	s->loopSound = 11;
	s->modelindex = 12;
	s->modelindex2 = 13;
	s->clientNum = 14;
	s->frame = 15;
	s->solid = 16;
	s->event = 17;
	s->eventParm = 18;
	s->powerups = 0xFF;
	s->weapon = 19;
	s->legsAnim = 20;
	s->torsoAnim = 21;
	s->generic1 = 22;
}

/* Compare every 32-bit word of two entity states for equality. */
static void assert_states_equal(const entityState_t *a, const entityState_t *b)
{
	const int *aw = (const int *)a;
	const int *bw = (const int *)b;
	int n = (int)(sizeof(*a) / 4);
	for (int i = 0; i < n; i++) {
		OA_ASSERT_INT(aw[i], bw[i]);
	}
}

TEST(msg_delta_entity_full_roundtrip)
{
	/* Write a full state using an all-zero baseline (every field differs,
	 * so everything is transmitted). Read it back and confirm every field
	 * survived exactly. */
	entityState_t base, in, out;
	Com_Memset(&base, 0, sizeof(base));
	fill_state(&in, 42);

	byte data[1024];
	msg_t buf;
	MSG_Init(&buf, data, sizeof(data));
	MSG_Bitstream(&buf);

	MSG_WriteDeltaEntity(&buf, &base, &in, qtrue);

	MSG_BeginReading(&buf);
	int num = MSG_ReadBits(&buf, GENTITYNUM_BITS);
	OA_ASSERT_INT(num, 42);
	Com_Memset(&out, 0, sizeof(out));
	MSG_ReadDeltaEntity(&buf, &base, &out, num);

	OA_ASSERT_INT(out.number, 42);
	assert_states_equal(&in, &out);
}

TEST(msg_delta_entity_partial_delta)
{
	/* Send the same baseline twice. The SECOND write is a delta from the
	 * first; only the fields that changed are transmitted, but the reader
	 * reconstructs the FULL state from the baseline. */
	entityState_t base, changed, out;
	fill_state(&base, 7);
	changed = base;
	changed.origin[0] = 99.0f;       /* only this field differs */
	changed.weapon = 123;            /* and this one */

	byte data[1024];
	msg_t buf;
	MSG_Init(&buf, data, sizeof(data));
	MSG_Bitstream(&buf);

	/* first: full write from an empty baseline */
	entityState_t empty;
	Com_Memset(&empty, 0, sizeof(empty));
	MSG_WriteDeltaEntity(&buf, &empty, &base, qtrue);
	/* second: delta from base */
	MSG_WriteDeltaEntity(&buf, &base, &changed, qfalse);

	MSG_BeginReading(&buf);
	/* read first entity (full) */
	int n1 = MSG_ReadBits(&buf, GENTITYNUM_BITS);
	entityState_t tmp;
	Com_Memset(&tmp, 0, sizeof(tmp));
	MSG_ReadDeltaEntity(&buf, &empty, &tmp, n1);
	/* read second entity (delta from base) */
	int n2 = MSG_ReadBits(&buf, GENTITYNUM_BITS);
	Com_Memset(&out, 0, sizeof(out));
	MSG_ReadDeltaEntity(&buf, &base, &out, n2);

	OA_ASSERT_INT(out.number, 7);
	/* unchanged fields come from the baseline, changed fields from the delta */
	OA_ASSERT_FLOAT(out.origin[0], 99.0f, 1e-3f);
	OA_ASSERT_INT(out.weapon, 123);
	OA_ASSERT_INT(out.eType, base.eType);
	OA_ASSERT_INT(out.modelindex, base.modelindex);
	assert_states_equal(&changed, &out);
}

TEST(msg_delta_entity_remove)
{
	/* Writing with to == NULL emits a remove message; reading it yields a
	 * cleared state with number == MAX_GENTITIES - 1. `from` must be a
	 * valid baseline (the engine dereferences it), so we pass the entity. */
	entityState_t in, out;
	fill_state(&in, 5);

	byte data[1024];
	msg_t buf;
	MSG_Init(&buf, data, sizeof(data));
	MSG_Bitstream(&buf);

	MSG_WriteDeltaEntity(&buf, &in, NULL, qfalse);

	MSG_BeginReading(&buf);
	int num = MSG_ReadBits(&buf, GENTITYNUM_BITS);
	OA_ASSERT_INT(num, 5);
	Com_Memset(&out, 0, sizeof(out));
	MSG_ReadDeltaEntity(&buf, &in, &out, num);

	OA_ASSERT_INT(out.number, MAX_GENTITIES - 1);
}

TEST(msg_delta_entity_no_change_is_empty)
{
	/* With force = qfalse and NO change from the baseline, WriteDeltaEntity
	 * writes nothing at all (the in-order delta code relies on this). So a
	 * subsequent read of the stream finds no entity. */
	entityState_t in;
	fill_state(&in, 3);

	byte data[1024];
	msg_t buf;
	MSG_Init(&buf, data, sizeof(data));
	MSG_Bitstream(&buf);

	int before = buf.cursize;
	MSG_WriteDeltaEntity(&buf, &in, &in, qfalse);  /* identical -> nothing */
	int after = buf.cursize;

	OA_ASSERT_INT(after, before);
}
