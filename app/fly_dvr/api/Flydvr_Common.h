//==============================================================================
//
//  File        : Flydvr_Common.h
//  Description : INCLUDE File for the FLY common function porting.
//  Author      :
//  Revision    : 1.0
//
//==============================================================================

#ifndef _FLYDVR_COMMON_H_
#define _FLYDVR_COMMON_H_


#include "lidbg_servicer.h"
#include "../../../drivers/inc/lidbg_flycam_par.h" /*flycam parameter*/

/*===========================================================================
 * Type define
 *===========================================================================*/
typedef unsigned char 		FLY_BOOL;
typedef unsigned char 		UINT8;
typedef unsigned short 		UINT16;
typedef unsigned int   		UINT;
typedef unsigned int  		UINT32;
typedef unsigned long 		ULONG;
typedef unsigned long*      PULONG; 
typedef unsigned long 		ULONG32;
typedef unsigned long long 	UINT64;
typedef unsigned char  		UBYTE;
typedef long long    		INT64;
typedef int                 INT;
typedef signed int   		INT32;
typedef signed short 		INT16;
typedef char         		INT8;
#ifndef BYTE
typedef unsigned char  		BYTE;
#endif
#ifndef PBYTE
typedef unsigned char*  	PBYTE;
#endif

/*===========================================================================
 * Macro define
 *===========================================================================*/

#define FLY_FAIL        (1)
#define FLY_SUCCESS     (0)

#define FLY_TRUE		(1)
#define FLY_FALSE		(0)

#endif		//_FLYDVR_COMMON_H_