// Bench.h

#ifndef __7ZIP_BENCH_H
#define __7ZIP_BENCH_H

#include "../../../Windows/System.h"

#include "../../Common/CreateCoder.h"
#include "../../UI/Common/Property.h"

#ifdef _7ZIP_LARGE_PAGES
void Add_LargePages_String(AString& s);
#else
// #define Add_LargePages_String
#endif

#endif
