/*
 * tests_qhuffman2.c - Unit tests for the low-level Huffman bit-stream and
 * tree primitives in qcommon/huffman.c that were still uncovered:
 *
 *   - Huff_Init            (initialises compressor/decompressor NYT tree)
 *   - Huff_getBloc / Huff_setBloc  (global bit-offset save/restore)
 *   - Huff_putBit / Huff_getBit    (raw bit stream into a byte buffer)
 *   - Huff_addRef          (grows the tree for a transmitted symbol)
 *
 * These are the SAFE low-level primitives (no NYT-deref crash). The
 * high-level Huff_offsetTransmit/Receive pair is intentionally NOT tested
 * here: without lock-step tree growth it dereferences loc[ch] with no NYT
 * handling and SIGSEGVs — the real round-trip path is covered by
 * tests_qhuffman.c (Huff_Compress/Huff_Decompress on a msg_t).
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qhuffman2.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_all
 */

#include "oa_test.h"

#include "q_shared.h"
#include "qcommon.h"

TEST(huffinit_creates_nyt_tree)
{
	huffman_t huff;
	Huff_Init(&huff);
	/* After init, the decompressor tree root is the NYT node (symbol 256). */
	OA_ASSERT_INT(huff.decompressor.tree->symbol, NYT);
	OA_ASSERT(huff.decompressor.tree != NULL);
	/* compressor root is also set */
	OA_ASSERT(huff.compressor.tree != NULL);
}

TEST(huffgetsetbloc)
{
	/* Huff_setBloc stores, Huff_getBloc retrieves the global bit offset. */
	Huff_setBloc(37);
	OA_ASSERT_INT(Huff_getBloc(), 37);
	Huff_setBloc(0);
	OA_ASSERT_INT(Huff_getBloc(), 0);
}

TEST(huffputgetbit_roundtrip)
{
	/* Writing a bit sequence then reading it back yields the same bits. */
	byte buf[8];
	int offset = 0;
	int i;
	Com_Memset(buf, 0, sizeof(buf));

	Huff_setBloc(0);
	Huff_putBit(1, buf, &offset);
	Huff_putBit(0, buf, &offset);
	Huff_putBit(1, buf, &offset);
	Huff_putBit(1, buf, &offset);
	OA_ASSERT_INT(offset, 4);

	offset = 0;
	Huff_setBloc(0);
	OA_ASSERT_INT(Huff_getBit(buf, &offset), 1);
	OA_ASSERT_INT(Huff_getBit(buf, &offset), 0);
	OA_ASSERT_INT(Huff_getBit(buf, &offset), 1);
	OA_ASSERT_INT(Huff_getBit(buf, &offset), 1);
	OA_ASSERT_INT(offset, 4);
}

TEST(huffputbit_packs_bytes)
{
	/* Eight 1-bits fill one byte with 0xFF. */
	byte buf[8];
	int offset = 0;
	int i;
	Com_Memset(buf, 0, sizeof(buf));
	Huff_setBloc(0);
	for (i = 0; i < 8; i++) {
		Huff_putBit(1, buf, &offset);
	}
	OA_ASSERT_INT(buf[0], 0xFF);
	OA_ASSERT_INT(offset, 8);
}

TEST(huffaddref_grows_tree)
{
	huffman_t huff;
	Huff_Init(&huff);
	/* Before addRef, loc['A'] is NULL (symbol never transmitted). */
	OA_ASSERT(huff.compressor.loc['A'] == NULL);
	Huff_addRef(&huff.compressor, 'A');
	/* After addRef, the symbol has a node in the compressor tree. */
	OA_ASSERT(huff.compressor.loc['A'] != NULL);
}
