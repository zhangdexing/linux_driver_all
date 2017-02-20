#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "debug.h"

#ifdef _DEBUG
static FILE *s_fp;

bool	DEBUG_INIT(void)
{
	// try to open an existing fifo file
	if ((s_fp = fopen(DEBUG_FIFO, "w")) == NULL)
	{
		perror("DEBUG_INIT : fopen() error!");
		return false;
	}
	return true;
}

void TRACE(const char *lpszFmt, ...)
{
	va_list arglist;
	char 	szBuf[256];
	int 	nBuf;

	if (s_fp == NULL)
		return;
	
	va_start(arglist, lpszFmt);

	nBuf = vsnprintf(szBuf, sizeof(szBuf), lpszFmt, arglist);
	if (nBuf > 0) 
	{
		fprintf(s_fp, "<trace> ");
		fprintf(s_fp, szBuf);
		fflush(s_fp);
	}

	va_end(arglist);
}

void DBG_Print(const char *lpszFmt, ...)
{
	va_list arglist;
	char 	szBuf[256];
	int 	nBuf;

	if (s_fp == NULL)
		return;
		
	va_start(arglist, lpszFmt);

	nBuf = vsnprintf(szBuf, sizeof(szBuf), lpszFmt, arglist);
	if (nBuf > 0) 
	{
		fprintf(s_fp, "<debug> ");
		fprintf(s_fp, szBuf);
		fflush(s_fp);
	}

	va_end(arglist);
}

#else

bool	DEBUG_INIT(void) { return true; }
void 	DBG_Print(const char *lpszFmt, ...) {}
void 	TRACE(const char *lpszFmt, ...) {}

#endif // _DEBUG
