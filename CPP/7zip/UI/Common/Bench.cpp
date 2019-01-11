// Bench.cpp

#include "../../../Common/Common.h"

#include <stdio.h>

#ifndef _WIN32
#  define USE_POSIX_TIME
#  define USE_POSIX_TIME2
#endif

#ifdef USE_POSIX_TIME
#  include <time.h>
#  ifdef USE_POSIX_TIME2
#    include <sys/time.h>
#  endif
#endif

#ifdef _WIN32
#  define USE_ALLOCA
#endif

#ifdef USE_ALLOCA
#  ifdef _WIN32
#    include <malloc.h>
#  else
#    include <stdlib.h>
#  endif
#endif

#include "../../../../C/7zCrc.h"
#include "../../../../C/Alloc.h"
#include "../../../../C/CpuArch.h"

#ifndef _7ZIP_ST
#  include "../../../Windows/Synchronization.h"
#  include "../../../Windows/Thread.h"
#endif

#if defined(_WIN32) || defined(UNIX_USE_WIN_FILE)
#  define USE_WIN_FILE
#endif

#ifdef USE_WIN_FILE
#  include "../../../Windows/FileIO.h"
#endif

#include "../../../Common/IntToString.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/StringToInt.h"

#include "../../Common/MethodProps.h"
#include "../../Common/StreamUtils.h"

#include "Bench.h"

using namespace NWindows;

static void PrintHex(AString& s, UInt64 v) {
  char temp[32];
  ConvertUInt64ToHex(v, temp);
  s += temp;
}
static void PrintSize(AString& s, UInt64 v) {
  char c = 0;
  if ((v & 0x3FF) == 0) {
    v >>= 10;
    c = 'K';
    if ((v & 0x3FF) == 0) {
      v >>= 10;
      c = 'M';
      if ((v & 0x3FF) == 0) {
        v >>= 10;
        c = 'G';
        if ((v & 0x3FF) == 0) {
          v >>= 10;
          c = 'T';
        }
      }
    }
  } else {
    PrintHex(s, v);
    return;
  }
  char temp[32];
  ConvertUInt64ToString(v, temp);
  s += temp;
  if (c) s += c;
}

#ifdef _7ZIP_LARGE_PAGES

extern bool g_LargePagesMode;

extern "C" {
extern SIZE_T g_LargePageSize;
}

void Add_LargePages_String(AString& s) {
  if (g_LargePagesMode || g_LargePageSize != 0) {
    s += " (LP-";
    PrintSize(s, g_LargePageSize);
    if (!g_LargePagesMode) s += "-NA";
    s += ")";
  }
}

#endif
