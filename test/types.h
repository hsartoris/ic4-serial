//============================================================================
//
//  File Name:     types.h
//  Description:   Needed for Unix versions
//  Created:       03/01/2002
//  Author:        Yury Altshuler
//
//  Copyright:     InterSense 2002 - All rights Reserved.
//
//
//=============================================================================
#ifndef _ISENSE_INC_types_h
#define _ISENSE_INC_types_h

#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__)

typedef BOOL                Bool;
typedef HWND                Hwnd;

#elif defined (__x86_64__)
typedef unsigned char       BYTE;
typedef int                 INT32;
typedef unsigned int        UINT32;
typedef unsigned int        DWORD;
typedef INT32               LONG;
typedef INT32               BOOL;
typedef INT32               Bool;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef long                Hwnd;

#else

typedef unsigned char       BYTE;
typedef unsigned long       DWORD;  
typedef long                LONG;
typedef long                Bool;
typedef unsigned short      WORD;
typedef long                Hwnd;

#endif
#endif
