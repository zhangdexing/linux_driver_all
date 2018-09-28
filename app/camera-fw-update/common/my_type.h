#ifndef _MY_TYPE_H
#define _MY_TYPE_H

#ifdef WIN32

#include <windows.h>

#pragma warning (disable : 4244)
#pragma warning (disable : 4554)

#define sleep(s)	Sleep(s*1000)
#define usleep		Sleep

#else

#include <sys/types.h>

typedef unsigned short	USHORT;
typedef unsigned char 	BYTE;
typedef unsigned short	WORD;
typedef unsigned int	DWORD;

#endif 

#endif	// _MY_TYPE_H

