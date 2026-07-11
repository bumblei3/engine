/*
 * test_stubs_cm_nofail.c - Like test_stubs_cm.c but Com_Error records the
 * error code and longjmp-s back (so the fatal/ERR_DROP path of
 * BaseWindingForPlane with a degenerate normal can be observed safely).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>

#include "q_shared.h"

int   oa_com_error_code = 0;
jmp_buf oa_com_error_jmp;

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
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	oa_com_error_code = code;
	longjmp(oa_com_error_jmp, 1);
}

void *Z_MallocDebug(int size, char *label, char *file, int line)
{
	(void)label;
	(void)file;
	(void)line;
	return malloc((size_t)size);
}

void Z_Free(void *ptr)
{
	free(ptr);
}

/* msg.c references the client CVar cl_shownet even when the delta-entity
 * reader isn't exercised; define it so the combined link succeeds. */
int cl_shownet = 0;
