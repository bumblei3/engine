/*
 * tests_qcm_poly_nofail.c - Fatal-path test for cm_polylib.c that calls
 * Com_Error (ERR_DROP) when given a degenerate winding. Runs in a
 * SEPARATE binary (test_cm_nofail) with the longjmp Com_Error stub
 * (test_stubs_cm_nofail.c) so the crash path is observed safely.
 *
 * Build & run:
 *   cd engine/tests
 *   gcc -I. -I../code/qcommon -I../code -o test_cm_nofail \
 *       oa_test_run.c test_main.c test_stubs_cm_nofail.c \
 *       tests_qcm_poly_nofail.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/cm_polylib.c -lm
 *   ./test_cm_nofail
 */

#include "oa_test.h"

#include "q_shared.h"
#include "cm_polylib.h"
#include <setjmp.h>

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

static void thunk_degenerate_winding(void)
{
	/* A winding with fewer than 3 points is degenerate and triggers
	 * Com_Error(ERR_DROP, ...) inside CheckWinding. */
	winding_t *w = AllocWinding(2);
	w->numpoints = 2;
	w->p[0][0] = 0.0f; w->p[0][1] = 0.0f; w->p[0][2] = 0.0f;
	w->p[1][0] = 1.0f; w->p[1][1] = 0.0f; w->p[1][2] = 0.0f;
	CheckWinding(w);
	FreeWinding(w);
}

TEST(checkwinding_degenerate_triggers_error)
{
	int code = oa_expect_error(thunk_degenerate_winding);
	OA_ASSERT_INT(code, ERR_DROP);
}
