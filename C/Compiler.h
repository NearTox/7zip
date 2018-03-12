/* Compiler.h
2015-08-02 : Igor Pavlov : Public domain */

#ifndef __7Z_COMPILER_H
#define __7Z_COMPILER_H

#ifdef _MSC_VER
#define NTDDI_VERSION   0x05010300

#define _WIN32_IE       0x0600
#define PSAPI_VERSION   1

//#define _WIN32_WINDOWS  0x0501
#define WINVER          0x0501
#define _WIN32_WINNT    0x0501
#ifdef UNDER_CE
#define RPC_NO_WINDOWS_H
#endif
#endif

#define UNUSED_VAR(x) (void)x;
/* #define UNUSED_VAR(x) x=x; */

#endif
