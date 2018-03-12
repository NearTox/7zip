// CWrappers.h

#ifndef __C_WRAPPERS_H
#define __C_WRAPPERS_H

#include "../ICoder.h"
#include "../../Common/MyCom.h"

struct CSeqInStreamWrap {
  ISeqInStream p;
  ISequentialInStream *Stream;
  HRESULT Res;
  UInt64 Processed;

  CSeqInStreamWrap(ISequentialInStream *stream) throw();
};

struct CSeekInStreamWrap {
  ISeekInStream p;
  IInStream *Stream;
  HRESULT Res;

  CSeekInStreamWrap(IInStream *stream) throw();
};

struct CSeqOutStreamWrap {
  ISeqOutStream p;
  ISequentialOutStream *Stream;
  HRESULT Res;
  UInt64 Processed;

  CSeqOutStreamWrap(ISequentialOutStream *stream) throw();
};

HRESULT SResToHRESULT(SRes res) throw();

struct CByteInBufWrap {
  IByteIn p;
  const Byte *Cur;
  const Byte *Lim;
  Byte *Buf;
  UInt32 Size;
  ISequentialInStream *Stream;
  UInt64 Processed;
  bool Extra;
  HRESULT Res;

  CByteInBufWrap();
  ~CByteInBufWrap() {
    Free();
  }
  void Free() throw();
  bool Alloc(UInt32 size) throw();
  void Init() {
    Lim = Cur = Buf;
    Processed = 0;
    Extra = false;
    Res = S_OK;
  }
  UInt64 GetProcessed() const {
    return Processed + (Cur - Buf);
  }
  Byte ReadByteFromNewBlock() throw();
  Byte ReadByte() {
    if(Cur != Lim)
      return *Cur++;
    return ReadByteFromNewBlock();
  }
};
#endif
