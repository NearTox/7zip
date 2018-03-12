// CreateCoder.h

#ifndef __CREATE_CODER_H
#define __CREATE_CODER_H

#include "../../Common/MyCom.h"
#include "../../Common/MyString.h"

#include "../ICoder.h"

#include "MethodId.h"

/*
  if EXTERNAL_CODECS is not defined, the code supports only codecs that
      are statically linked at compile-time and link-time.

  if EXTERNAL_CODECS is defined, the code supports also codecs from another
      executable modules, that can be linked dynamically at run-time:
        - EXE module can use codecs from external DLL files.
        - DLL module can use codecs from external EXE and DLL files.

      CExternalCodecs contains information about codecs and interfaces to create them.

  The order of codecs:
    1) Internal codecs
    2) External codecs
*/

bool FindMethod(const AString &name, CMethodId &methodId, UInt32 &numStreams);
bool FindMethod(CMethodId methodId, AString &name);
bool FindHashMethod(const AString &name, CMethodId &methodId);
void GetHashMethods(CRecordVector<CMethodId> &methods);

struct CCreatedCoder {
  CMyComPtr<ICompressCoder> Coder;
  CMyComPtr<ICompressCoder2> Coder2;

  bool IsExternal;
  bool IsFilter; // = true, if Coder was created from filter
  UInt32 NumStreams;

  // CCreatedCoder(): IsExternal(false), IsFilter(false), NumStreams(1) {}
};

HRESULT CreateCoder(CMethodId methodId, bool encode, CMyComPtr<ICompressFilter> &filter, CCreatedCoder &cod);
HRESULT CreateCoder(CMethodId methodId, bool encode, CCreatedCoder &cod);
HRESULT CreateCoder(CMethodId methodId, bool encode, CMyComPtr<ICompressCoder> &coder);
HRESULT CreateFilter(CMethodId methodId, bool encode, CMyComPtr<ICompressFilter> &filter);
HRESULT CreateHasher(CMethodId methodId, AString &name, CMyComPtr<IHasher> &hasher);

#endif
