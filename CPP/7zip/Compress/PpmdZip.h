// PpmdZip.h

#ifndef __COMPRESS_PPMD_ZIP_H
#define __COMPRESS_PPMD_ZIP_H

#include "../../../C/Alloc.h"
#include "../../../C/Ppmd8.h"

#include "../../Common/MyCom.h"

#include "../Common/CWrappers.h"

#include "../ICoder.h"

namespace NCompress {
  namespace NPpmdZip {
    static const UInt32 kBufSize = (1 << 20);

    struct CBuf {
      Byte *Buf;

      CBuf() : Buf(0) {}
      ~CBuf() {
        ::MidFree(Buf);
      }
      bool Alloc() {
        if(!Buf)
          Buf = (Byte *)::MidAlloc(kBufSize);
        return (Buf != 0);
      }
    };

    class CDecoder :
      public ICompressCoder,
      public CMyUnknownImp {
      CByteInBufWrap _inStream;
      CBuf _outStream;
      CPpmd8 _ppmd;
      bool _fullFileMode;
    public:
      MY_UNKNOWN_IMP
        STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
                        const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
      CDecoder(bool fullFileMode);
      ~CDecoder();
    };
  }
}

#endif
