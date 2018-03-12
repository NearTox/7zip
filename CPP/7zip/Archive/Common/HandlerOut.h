// HandlerOut.h

#ifndef __HANDLER_OUT_H
#define __HANDLER_OUT_H

#include "../../Common/MethodProps.h"

namespace NArchive {
  class CSingleMethodProps : public COneMethodInfo {
    UInt32 _level;

  public:
#ifndef _7ZIP_ST
    UInt32 _numThreads;
    UInt32 _numProcessors;
#endif

    void Init();
    CSingleMethodProps() {
      Init();
    }
    int GetLevel() const {
      return _level == (UInt32)(Int32)-1 ? 5 : (int)_level;
    }
    HRESULT SetProperties(const wchar_t * const *names, const PROPVARIANT *values, UInt32 numProps);
  };
}

#endif
