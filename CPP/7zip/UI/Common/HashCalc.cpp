// HashCalc.cpp

#include "../../../Common/Common.h"

#include "../../../../C/Alloc.h"

#include "../../../Common/StringToInt.h"

#include "../../Common/FileStreams.h"
#include "../../Common/StreamUtils.h"

#include "EnumDirItems.h"
#include "HashCalc.h"

using namespace NWindows;

class CHashMidBuf {
  void* _data;

 public:
  CHashMidBuf() : _data(0) {}
  operator void*() { return _data; }
  bool Alloc(size_t size) {
    if (_data != 0) return false;
    _data = ::MidAlloc(size);
    return _data != 0;
  }
  ~CHashMidBuf() { ::MidFree(_data); }
};

static const char* const k_DefaultHashMethod = "CRC32";

HRESULT CHashBundle::SetMethods( const UStringVector& hashMethods) {
  UStringVector names = hashMethods;
  if (names.IsEmpty()) names.Add(UString(k_DefaultHashMethod));

  CRecordVector<CMethodId> ids;
  CObjectVector<COneMethodInfo> methods;

  unsigned i;
  for (i = 0; i < names.Size(); i++) {
    COneMethodInfo m;
    RINOK(m.ParseMethodFromString(names[i]));

    if (m.MethodName.IsEmpty()) m.MethodName = k_DefaultHashMethod;

    if (m.MethodName == "*") {
      CRecordVector<CMethodId> tempMethods;
      GetHashMethods( tempMethods);
      methods.Clear();
      ids.Clear();
      FOR_VECTOR(t, tempMethods) {
        unsigned index = ids.AddToUniqueSorted(tempMethods[t]);
        if (ids.Size() != methods.Size()) methods.Insert(index, m);
      }
      break;
    } else {
      // m.MethodName.RemoveChar(L'-');
      CMethodId id;
      if (!FindHashMethod( m.MethodName, id)) return E_NOTIMPL;
      unsigned index = ids.AddToUniqueSorted(id);
      if (ids.Size() != methods.Size()) methods.Insert(index, m);
    }
  }

  for (i = 0; i < ids.Size(); i++) {
    CMyComPtr<IHasher> hasher;
    AString name;
    RINOK(CreateHasher( ids[i], name, hasher));
    if (!hasher) throw "Can't create hasher";
    const COneMethodInfo& m = methods[i];
    {
      CMyComPtr<ICompressSetCoderProperties> scp;
      hasher.QueryInterface(IID_ICompressSetCoderProperties, &scp);
      if (scp) RINOK(m.SetCoderProps(scp, NULL));
    }
    UInt32 digestSize = hasher->GetDigestSize();
    if (digestSize > k_HashCalc_DigestSize_Max) return E_NOTIMPL;
    CHasherState& h = Hashers.AddNew();
    h.Hasher = hasher;
    h.Name = name;
    h.DigestSize = digestSize;
    for (unsigned k = 0; k < k_HashCalc_NumGroups; k++) memset(h.Digests[k], 0, digestSize);
  }

  return S_OK;
}

void CHashBundle::InitForNewFile() {
  CurSize = 0;
  FOR_VECTOR(i, Hashers) {
    CHasherState& h = Hashers[i];
    h.Hasher->Init();
    memset(h.Digests[k_HashCalc_Index_Current], 0, h.DigestSize);
  }
}

void CHashBundle::Update(const void* data, UInt32 size) {
  CurSize += size;
  FOR_VECTOR(i, Hashers)
  Hashers[i].Hasher->Update(data, size);
}

void CHashBundle::SetSize(UInt64 size) { CurSize = size; }

static void AddDigests(Byte* dest, const Byte* src, UInt32 size) {
  unsigned next = 0;
  for (UInt32 i = 0; i < size; i++) {
    next += (unsigned)dest[i] + (unsigned)src[i];
    dest[i] = (Byte)next;
    next >>= 8;
  }
}

void CHashBundle::Final(bool isDir, bool isAltStream, const UString& path) {
  if (isDir)
    NumDirs++;
  else if (isAltStream) {
    NumAltStreams++;
    AltStreamsSize += CurSize;
  } else {
    NumFiles++;
    FilesSize += CurSize;
  }

  Byte pre[16];
  memset(pre, 0, sizeof(pre));
  if (isDir) pre[0] = 1;

  FOR_VECTOR(i, Hashers) {
    CHasherState& h = Hashers[i];
    if (!isDir) {
      h.Hasher->Final(h.Digests[0]);
      if (!isAltStream) AddDigests(h.Digests[k_HashCalc_Index_DataSum], h.Digests[0], h.DigestSize);
    }

    h.Hasher->Init();
    h.Hasher->Update(pre, sizeof(pre));
    h.Hasher->Update(h.Digests[0], h.DigestSize);

    for (unsigned k = 0; k < path.Len(); k++) {
      wchar_t c = path[k];
      Byte temp[2] = {(Byte)(c & 0xFF), (Byte)((c >> 8) & 0xFF)};
      h.Hasher->Update(temp, 2);
    }

    Byte tempDigest[k_HashCalc_DigestSize_Max];
    h.Hasher->Final(tempDigest);
    if (!isAltStream) AddDigests(h.Digests[k_HashCalc_Index_NamesSum], tempDigest, h.DigestSize);
    AddDigests(h.Digests[k_HashCalc_Index_StreamsSum], tempDigest, h.DigestSize);
  }
}

static inline char GetHex(unsigned v) { return (char)((v < 10) ? ('0' + v) : ('A' + (v - 10))); }

void AddHashHexToString(char* dest, const Byte* data, UInt32 size) {
  dest[size * 2] = 0;

  if (!data) {
    for (UInt32 i = 0; i < size; i++) {
      dest[0] = ' ';
      dest[1] = ' ';
      dest += 2;
    }
    return;
  }

  int step = 2;
  if (size <= 8) {
    step = -2;
    dest += size * 2 - 2;
  }

  for (UInt32 i = 0; i < size; i++) {
    unsigned b = data[i];
    dest[0] = GetHex((b >> 4) & 0xF);
    dest[1] = GetHex(b & 0xF);
    dest += step;
  }
}
