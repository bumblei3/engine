/*
 * test_stubs_nofail.c - Like test_stubs.c but Com_Error records the error
 * and LONGJMPs back to the test instead of aborting.
 *
 * Why longjmp and not a plain return: several engine functions call
 * Com_Error() on a fatal/oversize input and then KEEP RUNNING with that bad
 * buffer (they rely on Com_Error being fatal in production). A stub that only
 * records-and-returns would let them continue into a buffer over-read and
 * crash the test process. By longjmp-ing out, we simulate "Com_Error is fatal"
 * faithfully while still allowing the test to assert the guard fired.
 *
 * Tests arm the jump with setjmp(oa_com_error_jmp) before the call and check
 * oa_com_error_code afterwards. See tests_qinfo_guard.c.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <setjmp.h>

#include "q_shared.h"

int oa_com_error_code = 0;          /* last Com_Error code, observable by tests */
jmp_buf oa_com_error_jmp;           /* armed by tests via setjmp() */

void QDECL Com_Printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

void QDECL Com_DPrintf(const char *fmt, ...)
{
	(void)fmt;
}

void QDECL Com_Error(int code, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "Com_Error(nonfatal,%d): ", code);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	oa_com_error_code = code;
	/* Jump back to the test that armed the handler. longjmp never returns. */
	longjmp(oa_com_error_jmp, 1);
}

int QDECL FS_ReadFile(const char *qpath, void **buffer)
{
	(void)qpath; (void)buffer;
	return -1;
}

void QDECL FS_FreeFile(void *buffer)
{
	(void)buffer;
}

/* msg.c references this client CVar inside MSG_ReadDeltaEntity (unused by
 * the overflow tests); provide a definition so the TU links. */
int cl_shownet = 0;
