// PpmdZip.cpp

#include "../../Common/Common.h"
#include "../../../C/CpuArch.h"

#include "../Common/RegisterCodec.h"
#include "../Common/StreamUtils.h"

#include "PpmdZip.h"

namespace NCompress {
  namespace NPpmdZip {
    CDecoder::CDecoder(bool fullFileMode) :
      _fullFileMode(fullFileMode) {
      _ppmd.Stream.In = &_inStream.p;
      Ppmd8_Construct(&_ppmd);
    }

    CDecoder::~CDecoder() {
      Ppmd8_Free(&_ppmd, &g_BigAlloc);
    }

    STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
                                const UInt64 * /* inSize */, const UInt64 *outSize, ICompressProgressInfo *progress) {
      if(!_outStream.Alloc())
        return E_OUTOFMEMORY;
      if(!_inStream.Alloc(1 << 20))
        return E_OUTOFMEMORY;

      _inStream.Stream = inStream;
      _inStream.Init();

      {
        Byte buf[2];
        for(int i = 0; i < 2; i++)
          buf[i] = _inStream.ReadByte();
        if(_inStream.Extra)
          return S_FALSE;

        UInt32 val = GetUi16(buf);
        UInt32 order = (val & 0xF) + 1;
        UInt32 mem = ((val >> 4) & 0xFF) + 1;
        UInt32 restor = (val >> 12);
        if(order < 2 || restor > 2)
          return S_FALSE;

#ifndef PPMD8_FREEZE_SUPPORT
        if(restor == 2)
          return E_NOTIMPL;
#endif

        if(!Ppmd8_Alloc(&_ppmd, mem << 20, &g_BigAlloc))
          return E_OUTOFMEMORY;

        if(!Ppmd8_RangeDec_Init(&_ppmd))
          return S_FALSE;
        Ppmd8_Init(&_ppmd, order, restor);
      }

      bool wasFinished = false;
      UInt64 processedSize = 0;
      while(!outSize || processedSize < *outSize) {
        size_t size = kBufSize;
        if(outSize != nullptr) {
          const UInt64 rem = *outSize - processedSize;
          if(size > rem)
            size = (size_t)rem;
        }
        Byte *data = _outStream.Buf;
        size_t i = 0;
        int sym = 0;
        do {
          sym = Ppmd8_DecodeSymbol(&_ppmd);
          if(_inStream.Extra || sym < 0)
            break;
          data[i] = (Byte)sym;
        } while(++i != size);
        processedSize += i;

        RINOK(WriteStream(outStream, _outStream.Buf, i));

        RINOK(_inStream.Res);
        if(_inStream.Extra)
          return S_FALSE;

        if(sym < 0) {
          if(sym != -1)
            return S_FALSE;
          wasFinished = true;
          break;
        }
        if(progress) {
          UInt64 inSize = _inStream.GetProcessed();
          RINOK(progress->SetRatioInfo(&inSize, &processedSize));
        }
      }
      RINOK(_inStream.Res);
      if(_fullFileMode) {
        if(!wasFinished) {
          int res = Ppmd8_DecodeSymbol(&_ppmd);
          RINOK(_inStream.Res);
          if(_inStream.Extra || res != -1)
            return S_FALSE;
        }
        if(!Ppmd8_RangeDec_IsFinishedOK(&_ppmd))
          return S_FALSE;
      }
      return S_OK;
    }
  }
}