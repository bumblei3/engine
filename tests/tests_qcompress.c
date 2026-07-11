/*
 * tests_qcompress.c - Deeper unit tests for COM_Compress in
 * qcommon/q_shared.c. COM_Compress strips comments and collapses
 * whitespace, and is on the hot path for shader/script loading, so its
 * comment-stripping and newline handling must be locked down.
 *
 * Build & run (standalone, no SDL/OpenGL needed):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_compress \
 *       oa_test_run.c test_main.c test_stubs.c tests_qcompress.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_compress
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(com_compress_strips_block_comment)
{
	char buf[256];
	strcpy(buf, "a /* hidden */ b");
	int len = COM_Compress(buf);
	OA_ASSERT(len > 0);
	OA_ASSERT_STR(buf, "a b");
}

TEST(com_compress_strips_multiline_block_comment)
{
	char buf[256];
	strcpy(buf, "x /* line1\nline2\nline3 */ y");
	COM_Compress(buf);
	/* the block comment is removed entirely, leaving "x y" */
	OA_ASSERT_STR(buf, "x y");
}

TEST(com_compress_block_comment_then_line_comment)
{
	char buf[256];
	strcpy(buf, "v1 /* c */ v2 // tail\nv3");
	COM_Compress(buf);
	OA_ASSERT_STR(buf, "v1 v2\nv3");
}

TEST(com_compress_collapses_run_of_spaces)
{
	char buf[256];
	strcpy(buf, "a     b\t\tc");
	COM_Compress(buf);
	OA_ASSERT_STR(buf, "a b c");
}

TEST(com_compress_preserves_single_newline_between_tokens)
{
	char buf[256];
	strcpy(buf, "alpha\n\n\nbeta");
	COM_Compress(buf);
	/* multiple blank lines collapse to ONE retained newline */
	OA_ASSERT_STR(buf, "alpha\nbeta");
}

TEST(com_compress_preserves_quoted_strings)
{
	char buf[256];
	strcpy(buf, "say \"hello // not a comment\" end");
	COM_Compress(buf);
	/* the // inside quotes must survive, and quotes are kept verbatim */
	OA_ASSERT_STR(buf, "say \"hello // not a comment\" end");
}

TEST(com_compress_glsl_escape_disables_comment_strip)
{
	/* The engine supports a //GLSL... escape. It does NOT disable comment
	 * stripping for the whole line; it only skips the literal 6 characters
	 * "//GLSL" without treating them as a comment, then normal compression
	 * resumes. So a later // on the SAME line IS still stripped. Assert the
	 * ACTUAL behaviour so a regression is caught. */
	char buf[256];
	strcpy(buf, "//GLSL vec3 foo = bar; // trailing comment");
	COM_Compress(buf);
	/* "//GLSL" removed, whitespace collapsed, trailing // comment stripped */
	OA_ASSERT_STR(buf, " vec3 foo = bar;");
}

TEST(com_compress_glsl_escape_mid_string)
{
	char buf[256];
	strcpy(buf, "before //GLSL keep this // and this");
	COM_Compress(buf);
	/* only the //GLSL token is skipped; the following // starts a comment
	 * that eats " and this" */
	OA_ASSERT_STR(buf, "before keep this");
}

TEST(com_compress_empty_string)
{
	char buf[16] = "";
	int len = COM_Compress(buf);
	OA_ASSERT_INT(len, 0);
	OA_ASSERT_STR(buf, "");
}

TEST(com_compress_only_whitespace_becomes_empty)
{
	char buf[16] = "   \t\n  ";
	COM_Compress(buf);
	OA_ASSERT_STR(buf, "");
}
