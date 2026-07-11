/*
 * test_stubs_cm.c - Stubs for the cm_polylib.c winding-geometry tests.
 *
 * cm_polylib.c only needs a malloc-style allocator (AllocWinding/FreeWinding
 * call Z_Malloc/Z_Free) and the usual Com_Printf/Com_Error. We map Z_Malloc
 * to malloc and Z_Free to free, so the winding helpers link standalone
 * without the engine's zone memory manager. Com_Error is fatal (abort) here;
 * the ERR_DROP path of BaseWindingForPlane is covered separately in
 * test_stubs_cm_nofail.c (test_cm_nofail binary) with a longjmp stub.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "q_shared.h"

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
	fprintf(stderr, "Com_Error(%d): ", code);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	abort();
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
