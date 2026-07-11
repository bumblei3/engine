/*
 * tests_qparse_nofail.c - Fatal-path tests for the parse-session helpers in
 * qcommon/q_shared.c that call Com_Error (which is fatal in the engine).
 * These run in a SEPARATE binary (test_parse_nofail) with the non-aborting
 * Com_Error stub (test_stubs_nofail.c) that records the error code and
 * longjmp-s back to the test, so the fatal input can be observed safely.
 *
 *   - COM_MatchToken mismatch -> Com_Error(ERR_DROP, ...)
 *
 * Build & run (separate binary):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_parse_nofail \
 *       oa_test_run.c test_main.c test_stubs_nofail.c \
 *       tests_qparse_nofail.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_parse_nofail
 */

#include "oa_test.h"

#include "q_shared.h"
#include <setjmp.h>

/* The longjmp stub in test_stubs_nofail.c provides these. */
extern int   oa_com_error_code;
extern jmp_buf oa_com_error_jmp;

static int oa_expect_error(void (*fn)(void))
{
	oa_com_error_code = 0;
	if (setjmp(oa_com_error_jmp) == 0) {
		fn();
	}
	return oa_com_error_code;
}

static void thunk_matchtoken_mismatch(void)
{
	COM_BeginParseSession("script");
	char *p = (char *)"actual";
	/* token "actual" != expected "expected" -> Com_Error(ERR_DROP, ...) */
	COM_MatchToken(&p, "expected");
}

TEST(matchtoken_mismatch_triggers_error)
{
	int code = oa_expect_error(thunk_matchtoken_mismatch);
	OA_ASSERT_INT(code, ERR_DROP);
}
