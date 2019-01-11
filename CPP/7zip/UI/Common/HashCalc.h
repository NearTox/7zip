// HashCalc.h

#ifndef __HASH_CALC_H
#define __HASH_CALC_H

#include "../../../Common/Wildcard.h"

#include "../../Common/CreateCoder.h"
#include "../../Common/MethodProps.h"

#include "DirItem.h"

const unsigned k_HashCalc_DigestSize_Max = 64;

const unsigned k_HashCalc_NumGroups = 4;

enum {
  k_HashCalc_Index_Current,
  k_HashCalc_Index_DataSum,
  k_HashCalc_Index_NamesSum,
  k_HashCalc_Index_StreamsSum
};

struct CHasherState {
  CMyComPtr<IHasher> Hasher;
  AString Name;
  UInt32 DigestSize;
  Byte Digests[k_HashCalc_NumGroups][k_HashCalc_DigestSize_Max];
};

struct IHashCalc {
  virtual void InitForNewFile() = 0;
  virtual void Update(const void* data, UInt32 size) = 0;
  virtual void SetSize(UInt64 size) = 0;
  virtual void Final(bool isDir, bool isAltStream, const UString& path) = 0;
};

struct CHashBundle : public IHashCalc {
  CObjectVector<CHasherState> Hashers;

  UInt64 NumDirs;
  UInt64 NumFiles;
  UInt64 NumAltStreams;
  UInt64 FilesSize;
  UInt64 AltStreamsSize;
  UInt64 NumErrors;

  UInt64 CurSize;

  UString MainName;
  UString FirstFileName;

  HRESULT SetMethods( const UStringVector& methods);

  // void Init() {}
  CHashBundle() { NumDirs = NumFiles = NumAltStreams = FilesSize = AltStreamsSize = NumErrors = 0; }

  void InitForNewFile();
  void Update(const void* data, UInt32 size);
  void SetSize(UInt64 size);
  void Final(bool isDir, bool isAltStream, const UString& path);
};

void AddHashHexToString(char* dest, const Byte* data, UInt32 size);

#endif
