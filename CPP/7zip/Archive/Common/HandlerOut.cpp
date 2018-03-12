// HandlerOut.cpp

#include "../../../Common/Common.h"

#ifndef _7ZIP_ST
#include "../../../Windows/System.h"
#endif
#include "HandlerOut.h"

using namespace NWindows;

namespace NArchive {
  static void SetMethodProp32(COneMethodInfo &m, PROPID propID, UInt32 value) {
    if(m.FindProp(propID) < 0)
      m.AddProp32(propID, value);
  }

  void CSingleMethodProps::Init() {
    Clear();
    _level = (UInt32)(Int32)-1;

#ifndef _7ZIP_ST
    _numProcessors = _numThreads = NWindows::NSystem::GetNumberOfProcessors();
    AddProp_NumThreads(_numThreads);
#endif
  }

  HRESULT CSingleMethodProps::SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps) {
    Init();
    for(UInt32 i = 0; i < numProps; i++) {
      UString name = names[i];
      name.MakeLower_Ascii();
      if(name.IsEmpty())
        return E_INVALIDARG;
      const PROPVARIANT &value = values[i];
      if(name[0] == L'x') {
        UInt32 a = 9;
        RINOK(ParsePropToUInt32(name.Ptr(1), value, a));
        _level = a;
        AddProp_Level(a);
      } else if(name.IsPrefixedBy_Ascii_NoCase("mt")) {
#ifndef _7ZIP_ST
        RINOK(ParseMtProp(name.Ptr(2), value, _numProcessors, _numThreads));
        AddProp_NumThreads(_numThreads);
#endif
      } else {
        RINOK(ParseMethodFromPROPVARIANT(names[i], value));
      }
    }
    return S_OK;
  }
}