// HandlerOut.h

#ifndef __HANDLER_OUT_H
#define __HANDLER_OUT_H

#include "../../../Windows/System.h"

#include "../../Common/MethodProps.h"

namespace NArchive {

bool ParseSizeString(
    const wchar_t* name, const PROPVARIANT& prop, UInt64 percentsBase, UInt64& res);

class CCommonMethodProps {
 protected:
  void InitCommon() {
#ifndef _7ZIP_ST
    _numProcessors = _numThreads = NWindows::NSystem::GetNumberOfProcessors();
#endif

    UInt64 memAvail = (UInt64)(sizeof(size_t)) << 28;
    _memAvail = memAvail;
    _memUsage = memAvail;
    if (NWindows::NSystem::GetRamSize(memAvail)) {
      _memAvail = memAvail;
      _memUsage = memAvail / 32 * 17;
    }
  }

 public:
#ifndef _7ZIP_ST
  UInt32 _numThreads;
  UInt32 _numProcessors;
#endif

  UInt64 _memUsage;
  UInt64 _memAvail;

  bool SetCommonProperty(const UString& name, const PROPVARIANT& value, HRESULT& hres);

  CCommonMethodProps() { InitCommon(); }
};

}  // namespace NArchive

#endif
