/*
 * tests_qparse2.c - Deeper unit tests for the token parser / brace skipper in
 * qcommon/q_shared.c (COM_ParseExt allowLineBreaks behaviour, SkipBracedSection,
 * SkipRestOfLine, Parse1D/2D/3DMatrix). These helpers drive shader and config
 * loading, where mishandled braces or line breaks are high-impact.
 *
 * Build & run (standalone, no SDL/OpenGL needed):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_parse2 \
 *       oa_test_run.c test_main.c test_stubs.c tests_qparse2.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_parse2
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(com_parse_ext_allow_line_breaks_true_consumes_newline)
{
	/* With allowLineBreaks=qtrue, a newline is treated as whitespace and
	 * consumed; the next token continues on the next line with no empty
	 * token in between. */
	char buf[] = "a\nb";
	char *p = buf;
	OA_ASSERT_STR(COM_ParseExt(&p, qtrue), "a");
	OA_ASSERT_STR(COM_ParseExt(&p, qtrue), "b");
	OA_ASSERT_STR(COM_ParseExt(&p, qtrue), "");
}

TEST(com_parse_ext_disallow_line_breaks_stops_at_newline)
{
	/* With allowLineBreaks=qfalse, a newline stops parsing: an empty token
	 * is returned AND the cursor is left just after the newline (the
	 * newline itself was already consumed by the whitespace skipper). */
	char buf[] = "a\nb";
	char *p = buf;
	OA_ASSERT_STR(COM_ParseExt(&p, qfalse), "a");
	OA_ASSERT_STR(COM_ParseExt(&p, qfalse), "");
	/* p points at the first token after the newline */
	OA_ASSERT_STR(p, "b");
}

TEST(com_parse_ext_disallow_yields_empty_token_before_newline)
{
	/* A leading newline with line breaks disallowed yields an empty token
	 * immediately. */
	char buf[] = "\nvalue";
	char *p = buf;
	OA_ASSERT_STR(COM_ParseExt(&p, qfalse), "");
	OA_ASSERT_STR(COM_ParseExt(&p, qfalse), "value");
}

TEST(skip_braced_section_simple)
{
	char buf[] = "{ inner } after";
	char *p = buf;
	qboolean ok = SkipBracedSection(&p, 0);
	/* the whole { ... } is consumed, leaving " after" */
	OA_ASSERT(ok == qtrue);
	OA_ASSERT_STR(p, " after");
}

TEST(skip_braced_section_nested)
{
	char buf[] = "{ a { b { c } d } e } tail";
	char *p = buf;
	qboolean ok = SkipBracedSection(&p, 0);
	/* nested braces are tracked; only the matching top-level close counts */
	OA_ASSERT(ok == qtrue);
	OA_ASSERT_STR(p, " tail");
}

TEST(skip_braced_section_unbalanced_returns_false)
{
	/* missing close brace: depth never reaches 0, function returns qfalse
	 * but must not run off into infinite loop (stops at end of string). */
	char buf[] = "{ unclosed";
	char *p = buf;
	qboolean ok = SkipBracedSection(&p, 0);
	OA_ASSERT(ok == qfalse);
}

TEST(skip_braced_section_starts_inside_with_depth)
{
	/* caller already consumed the opening brace and passes depth=1 */
	char buf[] = " nested } more";
	char *p = buf;
	qboolean ok = SkipBracedSection(&p, 1);
	OA_ASSERT(ok == qtrue);
	OA_ASSERT_STR(p, " more");
}

TEST(skip_rest_of_line_stops_after_newline)
{
	char buf[] = "junk more junk\nnext line";
	char *p = buf;
	SkipRestOfLine(&p);
	/* cursor is left just past the newline */
	OA_ASSERT_STR(p, "next line");
}

TEST(skip_rest_of_line_at_eof)
{
	char buf[] = "only one line";
	char *p = buf;
	SkipRestOfLine(&p);
	/* end of string: cursor points at the terminator */
	OA_ASSERT_INT(*p, 0);
}

TEST(parse_1d_matrix_roundtrip)
{
	/* Parse1DMatrix expects "( f0 f1 f2 )" and writes floats. */
	char buf[] = "( 1.0 2.5 3.25 )";
	char *p = buf;
	float m[3];
	Parse1DMatrix(&p, 3, m);
	OA_ASSERT_FLOAT(m[0], 1.0f, 1e-6);
	OA_ASSERT_FLOAT(m[1], 2.5f, 1e-6);
	OA_ASSERT_FLOAT(m[2], 3.25f, 1e-6);
	/* cursor left just after the closing paren */
	OA_ASSERT_STR(p, "");
}

TEST(parse_2d_matrix_roundtrip)
{
	char buf[] = "( ( 1 2 ) ( 3 4 ) )";
	char *p = buf;
	float m[4];
	Parse2DMatrix(&p, 2, 2, m);
	OA_ASSERT_FLOAT(m[0], 1.0f, 1e-6);
	OA_ASSERT_FLOAT(m[1], 2.0f, 1e-6);
	OA_ASSERT_FLOAT(m[2], 3.0f, 1e-6);
	OA_ASSERT_FLOAT(m[3], 4.0f, 1e-6);
}

TEST(parse_3d_matrix_roundtrip)
{
	char buf[] = "( ( ( 1 ) ( 2 ) ) ( ( 3 ) ( 4 ) ) )";
	char *p = buf;
	float m[4];
	Parse3DMatrix(&p, 2, 2, 1, m);
	OA_ASSERT_FLOAT(m[0], 1.0f, 1e-6);
	OA_ASSERT_FLOAT(m[1], 2.0f, 1e-6);
	OA_ASSERT_FLOAT(m[2], 3.0f, 1e-6);
	OA_ASSERT_FLOAT(m[3], 4.0f, 1e-6);
}
