// HandlerOut.cpp

#include "../../../Common/Common.h"

#include "../../../Common/StringToInt.h"

#include "HandlerOut.h"

namespace NArchive {

bool ParseSizeString(const wchar_t* s, const PROPVARIANT& prop, UInt64 percentsBase, UInt64& res) {
  if (*s == 0) {
    switch (prop.vt) {
      case VT_UI4: res = prop.ulVal; return true;
      case VT_UI8: res = prop.uhVal.QuadPart; return true;
      case VT_BSTR: s = prop.bstrVal; break;
      default: return false;
    }
  } else if (prop.vt != VT_EMPTY)
    return false;

  const wchar_t* end;
  UInt64 v = ConvertStringToUInt64(s, &end);
  if (s == end) return false;
  wchar_t c = *end;
  if (c == 0) {
    res = v;
    return true;
  }
  if (end[1] != 0) return false;

  if (c == '%') {
    res = percentsBase / 100 * v;
    return true;
  }

  unsigned numBits;
  switch (MyCharLower_Ascii(c)) {
    case 'b': numBits = 0; break;
    case 'k': numBits = 10; break;
    case 'm': numBits = 20; break;
    case 'g': numBits = 30; break;
    case 't': numBits = 40; break;
    default: return false;
  }
  UInt64 val2 = v << numBits;
  if ((val2 >> numBits) != v) return false;
  res = val2;
  return true;
}

bool CCommonMethodProps::SetCommonProperty(
    const UString& name, const PROPVARIANT& value, HRESULT& hres) {
  hres = S_OK;

  if (name.IsPrefixedBy_Ascii_NoCase("mt")) {
#ifndef _7ZIP_ST
    hres = ParseMtProp(name.Ptr(2), value, _numProcessors, _numThreads);
#endif
    return true;
  }

  if (name.IsPrefixedBy_Ascii_NoCase("memuse")) {
    if (!ParseSizeString(name.Ptr(6), value, _memAvail, _memUsage)) hres = E_INVALIDARG;
    return true;
  }

  return false;
}

}  // namespace NArchive
