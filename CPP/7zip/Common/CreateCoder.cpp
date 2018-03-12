// CreateCoder.cpp

#include "../../Common/Common.h"

#include "../../Windows/Defs.h"
#include "../../Windows/PropVariant.h"

#include "CreateCoder.h"

#include "FilterCoder.h"
#include "RegisterCodec.h"

static const unsigned kNumCodecsMax = 64;
unsigned g_NumCodecs = 0;
const CCodecInfo *g_Codecs[kNumCodecsMax];

#define CHECK_GLOBAL_CODECS

void RegisterCodec(const CCodecInfo *codecInfo) throw() {
  if(g_NumCodecs < kNumCodecsMax)
    g_Codecs[g_NumCodecs++] = codecInfo;
}

static const unsigned kNumHashersMax = 16;
unsigned g_NumHashers = 0;
const CHasherInfo *g_Hashers[kNumHashersMax];

void RegisterHasher(const CHasherInfo *hashInfo) throw() {
  if(g_NumHashers < kNumHashersMax)
    g_Hashers[g_NumHashers++] = hashInfo;
}

bool FindMethod(const AString &name, CMethodId &methodId, UInt32 &numStreams) {
  unsigned i;
  for(i = 0; i < g_NumCodecs; i++) {
    const CCodecInfo &codec = *g_Codecs[i];
    if(StringsAreEqualNoCase_Ascii(name, codec.Name)) {
      methodId = codec.Id;
      numStreams = codec.NumStreams;
      return true;
    }
  }
  return false;
}

bool FindMethod(CMethodId methodId, AString &name) {
  name.Empty();

  unsigned i;
  for(i = 0; i < g_NumCodecs; i++) {
    const CCodecInfo &codec = *g_Codecs[i];
    if(methodId == codec.Id) {
      name = codec.Name;
      return true;
    }
  }
  return false;
}

bool FindHashMethod(const AString &name, CMethodId &methodId) {
  unsigned i;
  for(i = 0; i < g_NumHashers; i++) {
    const CHasherInfo &codec = *g_Hashers[i];
    if(StringsAreEqualNoCase_Ascii(name, codec.Name)) {
      methodId = codec.Id;
      return true;
    }
  }
  return false;
}

void GetHashMethods(CRecordVector<CMethodId> &methods) {
  methods.ClearAndSetSize(g_NumHashers);
  unsigned i;
  for(i = 0; i < g_NumHashers; i++)
    methods[i] = (*g_Hashers[i]).Id;
}

HRESULT CreateCoder(CMethodId methodId, bool encode, CMyComPtr<ICompressFilter> &filter, CCreatedCoder &cod) {
  cod.IsExternal = false;
  cod.IsFilter = false;
  cod.NumStreams = 1;

  unsigned i;
  for(i = 0; i < g_NumCodecs; i++) {
    const CCodecInfo &codec = *g_Codecs[i];
    if(codec.Id == methodId) {
      if(encode) {
        /*if(codec.CreateEncoder) {
          void *p = codec.CreateEncoder();
          if(codec.IsFilter) filter = (ICompressFilter *)p;
          else if(codec.NumStreams == 1) cod.Coder = (ICompressCoder *)p;
          else {
            cod.Coder2 = (ICompressCoder2 *)p; cod.NumStreams = codec.NumStreams;
          }
          return S_OK;
        }*/
      } else
        if(codec.CreateDecoder) {
          void *p = codec.CreateDecoder();
          if(codec.IsFilter) filter = (ICompressFilter *)p;
          else if(codec.NumStreams == 1) cod.Coder = (ICompressCoder *)p;
          else {
            cod.Coder2 = (ICompressCoder2 *)p; cod.NumStreams = codec.NumStreams;
          }
          return S_OK;
        }
    }
  }

  return S_OK;
}

HRESULT CreateCoder(CMethodId methodId, bool encode, CCreatedCoder &cod) {
  CMyComPtr<ICompressFilter> filter;
  HRESULT res = CreateCoder(methodId, encode, filter, cod);

  if(filter) {
    cod.IsFilter = true;
    CFilterCoder *coderSpec = new CFilterCoder(encode);
    cod.Coder = coderSpec;
    coderSpec->Filter = filter;
  }

  return res;
}

HRESULT CreateCoder(CMethodId methodId, bool encode, CMyComPtr<ICompressCoder> &coder) {
  CCreatedCoder cod;
  HRESULT res = CreateCoder(methodId, encode, cod);
  coder = cod.Coder;
  return res;
}

HRESULT CreateFilter(

  CMethodId methodId, bool encode,
  CMyComPtr<ICompressFilter> &filter) {
  CCreatedCoder cod;
  return CreateCoder(

    methodId, encode,
    filter, cod);
}

HRESULT CreateHasher(

  CMethodId methodId,
  AString &name,
  CMyComPtr<IHasher> &hasher) {
  name.Empty();

  unsigned i;
  for(i = 0; i < g_NumHashers; i++) {
    const CHasherInfo &codec = *g_Hashers[i];
    if(codec.Id == methodId) {
      hasher = codec.CreateHasher();
      name = codec.Name;
      break;
    }
  }
  return S_OK;
}