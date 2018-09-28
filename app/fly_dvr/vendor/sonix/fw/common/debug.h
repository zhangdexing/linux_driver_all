#ifndef _DEBUG_H
#define _DEBUG_H

// #define	_DEBUG

#define ENABLE_DEBUG	1
#define ENABLE_TRACE	1

#define DEBUG_FIFO 		"/root/DEBUG_FIFO"

bool	DEBUG_INIT(void);
void 	DBG_Print(const char *lpszFmt, ...);
void 	TRACE(const char *lpszFmt, ...);

#endif

