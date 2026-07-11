/*
 * tests_qshared3.c - Unit tests for the parse-session / error-reporting
 * helpers in qcommon/q_shared.c that were still uncovered:
 *
 *   - Q_vsnprintf        (vsnprintf wrapper; truncation clamps to size-1 NUL)
 *   - COM_BeginParseSession / COM_GetCurrentParseLine (parse-line tracking)
 *   - COM_MatchToken     (success path: token equals match)
 *   - COM_ParseError / COM_ParseWarning (formatted reporting via Com_Printf)
 *
 * The COM_MatchToken MISMATCH path calls Com_Error and is covered separately
 * in the nofail binary (tests_qparse_nofail.c -> test_parse_nofail), so it
 * does not abort the main suite. All these are standalone-linkable with
 * q_shared.c (Com_Printf/Com_Error are stubbed).
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared3.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_all
 */

#include "oa_test.h"

#include "q_shared.h"
#include <stdarg.h>

/* Q_vsnprintf is declared in q_shared.h (line ~170). */

/* Helper to call Q_vsnprintf through a real va_list. */
static int call_vsnprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int r = Q_vsnprintf(buf, size, fmt, ap);
	va_end(ap);
	return r;
}

TEST(qvsnprintf_basic)
{
	char buf[64];
	int r = call_vsnprintf(buf, sizeof(buf), "%s = %d", "score", 42);
	/* "score = 42" is 10 chars */
	OA_ASSERT_STR(buf, "score = 42");
	OA_ASSERT_INT(r, 10);
}

TEST(qvsnprintf_truncation_clamps)
{
	/* size smaller than the formatted output: glibc's vsnprintf writes what
	 * fits (size-1 bytes) and NUL-terminates at size-1, returning the WOULD-BE
	 * length (10). The engine's retval==size MS-workaround branch only fires
	 * on platforms whose vsnprintf returns size/negative on truncation; here
	 * we assert the portable observable contract: buffer is NUL-terminated. */
	char buf[8];
	int r = call_vsnprintf(buf, sizeof(buf), "%s", "0123456789");
	/* buffer holds 7 chars + NUL at index 7 */
	OA_ASSERT_INT(buf[7], 0);
	OA_ASSERT_INT(r, 10);
}

TEST(qvsnprintf_float)
{
	char buf[64];
	int r = call_vsnprintf(buf, sizeof(buf), "%.2f", 3.14159);
	OA_ASSERT_STR(buf, "3.14");
	OA_ASSERT_INT(r, 4);
}

TEST(beginparsesession_sets_name_and_line)
{
	COM_BeginParseSession("mymap.bsp");
	/* after a fresh session, line counter is 1 and tokenline 0, so
	 * COM_GetCurrentParseLine returns com_lines == 1. */
	OA_ASSERT_INT(COM_GetCurrentParseLine(), 1);
}

TEST(getcurrentparseline_tracks_tokens)
{
	/* Parsing tokens advances com_lines; COM_GetCurrentParseLine reflects it. */
	COM_BeginParseSession("script");
	const char *src = "a\nb\nc";
	char *p = (char *)src;
	COM_Parse(&p);          /* "a" -> line 1 */
	COM_Parse(&p);          /* "b" -> line 2 (newline advanced com_lines) */
	/* After consuming the newline before "b", com_lines is 2. */
	OA_ASSERT_INT(COM_GetCurrentParseLine(), 2);
}

TEST(matchtoken_success)
{
	/* Matching token: no Com_Error, parsing advances past the token. */
	char *p = (char *)"hello world";
	COM_MatchToken(&p, "hello");
	/* remaining text is " world" (leading space preserved as whitespace) */
	OA_ASSERT_INT(*p, ' ');
}

TEST(parseerror_formats_and_returns)
{
	/* COM_ParseError only logs via Com_Printf (stubbed); it does not abort.
	 * We just assert it returns normally after a session was begun. */
	COM_BeginParseSession("cfg");
	COM_ParseError("bad value %d", 7);
	COM_ParseWarning("odd token %s", "x");
	OA_ASSERT(1);  /* reached without abort */
}
