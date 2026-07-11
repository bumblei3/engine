/*
 * tests_qinfo_guard.c - Guard-path tests for the info-string helpers.
 *
 * Several info functions call Com_Error(ERR_DROP, ...) on oversize input
 * (Info_ValueForKey, Info_RemoveKey, Info_SetValueForKey, Info_RemoveKey_Big,
 * Info_SetValueForKey_Big). In the normal test binary Com_Error aborts, so
 * these fatal paths are exercised only here, where test_stubs_nofail.c
 * records the error code and longjmps back instead of aborting. The suite
 * then asserts that the guard actually fired.
 *
 * Compiled into the SEPARATE test_overflow binary (shares the non-aborting
 * stub), so it does not disturb the aborting suites.
 *
 * Build & run (standalone, no SDL/OpenGL needed):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_overflow \
 *       oa_test_run.c test_main.c test_stubs_nofail.c \
 *       tests_qinfo_overflow.c tests_qinfo_guard.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_overflow
 */
#include "oa_test.h"

#include "q_shared.h"

#include <setjmp.h>

/* Provided by test_stubs_nofail.c — Com_Error longjmps here after recording
 * the error code, faithfully simulating the engine's fatal error handling
 * without killing the test process. */
extern int  oa_com_error_code;
extern jmp_buf oa_com_error_jmp;

/* Each guard test needs a thunk (plain function pointer, no compiler
 * blocks) so oa_expect_error can invoke it inside the setjmp envelope.
 * g_buf is sized for the largest guard input (BIG_INFO_STRING) so the
 * BIG_INFO_STRING-filling thunks do not overflow it. */
static char g_buf[BIG_INFO_STRING + 8];

static void thunk_valueforkey(void)     { (void)Info_ValueForKey(g_buf, "x"); }
static void thunk_removekey(void)       { Info_RemoveKey(g_buf, "k"); }
static void thunk_setvalue(void)        { Info_SetValueForKey(g_buf, "k", "v"); }
static void thunk_removekey_big(void)   { Info_RemoveKey_Big(g_buf, "k"); }
static void thunk_setvalue_big(void)    { Info_SetValueForKey_Big(g_buf, "k", "v"); }

/* Arm the Com_Error jump and call FN. Returns the Com_Error code that fired
 * (0 if FN returned normally). */
static int oa_expect_error(void (*fn)(void))
{
	oa_com_error_code = 0;
	if (setjmp(oa_com_error_jmp) == 0) {
		fn();
	}
	return oa_com_error_code;
}

TEST(info_valueforkey_oversize_triggers_error)
{
	/* Info_ValueForKey guards against BIG_INFO_STRING (not MAX_INFO_STRING),
	 * so the oversize input must reach BIG_INFO_STRING to trip it. */
	memset(g_buf, 'a', BIG_INFO_STRING);
	g_buf[BIG_INFO_STRING] = '\0';

	int code = oa_expect_error(thunk_valueforkey);
	OA_ASSERT_INT(code, ERR_DROP);
}

TEST(info_valueforkey_null_key_safe)
{
	/* NULL s or NULL key returns "" without erroring. */
	oa_com_error_code = 0;
	OA_ASSERT_STR(Info_ValueForKey(NULL, "k"), "");
	OA_ASSERT_STR(Info_ValueForKey("foo", NULL), "");
	OA_ASSERT_INT(oa_com_error_code, 0);
}

TEST(info_removekey_oversize_triggers_error)
{
	memset(g_buf, 'a', MAX_INFO_STRING);
	g_buf[MAX_INFO_STRING] = '\0';

	int code = oa_expect_error(thunk_removekey);
	OA_ASSERT_INT(code, ERR_DROP);
}

TEST(info_setvalue_oversize_triggers_error)
{
	memset(g_buf, 'a', MAX_INFO_STRING);
	g_buf[MAX_INFO_STRING] = '\0';

	int code = oa_expect_error(thunk_setvalue);
	OA_ASSERT_INT(code, ERR_DROP);
}

TEST(info_removekey_big_oversize_triggers_error)
{
	memset(g_buf, 'a', BIG_INFO_STRING);
	g_buf[BIG_INFO_STRING] = '\0';

	int code = oa_expect_error(thunk_removekey_big);
	OA_ASSERT_INT(code, ERR_DROP);
}

TEST(info_setvalue_big_oversize_triggers_error)
{
	memset(g_buf, 'a', BIG_INFO_STRING);
	g_buf[BIG_INFO_STRING] = '\0';

	int code = oa_expect_error(thunk_setvalue_big);
	OA_ASSERT_INT(code, ERR_DROP);
}

TEST(info_setvalue_rejects_blacklisted_key)
{
	/* keys/values containing '\\', ';' or '\"' are rejected (return, no
	 * error, no write). The guard uses Com_Printf, not Com_Error, so the
	 * call returns normally. */
	char s[MAX_INFO_STRING];
	strcpy(s, "\\name\\x");
	oa_com_error_code = 0;

	Info_SetValueForKey(s, "bad\\key", "v");
	/* string unchanged: the bad key was not added */
	OA_ASSERT_STR(s, "\\name\\x");
	OA_ASSERT_INT(oa_com_error_code, 0);
}
