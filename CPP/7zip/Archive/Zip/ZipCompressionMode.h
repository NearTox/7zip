// CompressionMode.h

#ifndef __ZIP_COMPRESSION_MODE_H
#define __ZIP_COMPRESSION_MODE_H

#include "../../../Common/MyString.h"

#ifndef _7ZIP_ST
#include "../../../Windows/System.h"
#endif

#include "../Common/HandlerOut.h"

namespace NArchive {
  namespace NZip {
    struct CBaseProps {
      Int32 Level;

#ifndef _7ZIP_ST
      UInt32 NumThreads;
      bool NumThreadsWasChanged;
#endif
      bool IsAesMode;
      Byte AesKeyMode;

      void Init() {
        Level = -1;
#ifndef _7ZIP_ST
        NumThreads = NWindows::NSystem::GetNumberOfProcessors();;
        NumThreadsWasChanged = false;
#endif
        IsAesMode = false;
        AesKeyMode = 3;
      }
    };
  }
}

#endif
