// Update.cpp

#include "../../../Common/Common.h"

#include "Update.h"

#include "../../../Common/StringConvert.h"

#include "../../../Windows/DLL.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileFind.h"
#include "../../../Windows/FileName.h"
#include "../../../Windows/PropVariant.h"
#include "../../../Windows/PropVariantConv.h"
#include "../../../Windows/TimeUtils.h"

#include "../../Common/FileStreams.h"
#include "../../Common/LimitedStreams.h"

#include "../../Compress/CopyCoder.h"

#include "../Common/DirItem.h"
#include "../Common/EnumDirItems.h"
#include "../Common/OpenArchive.h"

#include "EnumDirItems.h"
#include "SetProperties.h"
#include "TempFiles.h"

static const char* const kUpdateIsNotSupoorted =
    "update operations are not supported for this archive";

static const char* const kUpdateIsNotSupoorted_MultiVol =
    "Updating for multivolume archives is not implemented";

using namespace NWindows;
using namespace NCOM;
using namespace NFile;
using namespace NDir;
using namespace NName;

static CFSTR const kTempFolderPrefix = FTEXT("7zE");

static bool DeleteEmptyFolderAndEmptySubFolders(const FString& path) {
  NFind::CFileInfo fileInfo;
  FString pathPrefix = path + FCHAR_PATH_SEPARATOR;
  {
    NFind::CEnumerator enumerator;
    enumerator.SetDirPrefix(pathPrefix);
    while (enumerator.Next(fileInfo)) {
      if (fileInfo.IsDir())
        if (!DeleteEmptyFolderAndEmptySubFolders(pathPrefix + fileInfo.Name)) return false;
    }
  }
  /*
  // we don't need clear read-only for folders
  if (!MySetFileAttributes(path, 0))
    return false;
  */
  return RemoveDir(path);
}

class COutMultiVolStream
    : public IOutStream
    , public CMyUnknownImp {
  unsigned _streamIndex;  // required stream
  UInt64 _offsetPos;      // offset from start of _streamIndex index
  UInt64 _absPos;
  UInt64 _length;

  struct CAltStreamInfo {
    COutFileStream* StreamSpec;
    CMyComPtr<IOutStream> Stream;
    FString Name;
    UInt64 Pos;
    UInt64 RealSize;
  };
  CObjectVector<CAltStreamInfo> Streams;

 public:
  // CMyComPtr<IArchiveUpdateCallback2> VolumeCallback;
  CRecordVector<UInt64> Sizes;
  FString Prefix;
  CTempFiles* TempFiles;

  void Init() {
    _streamIndex = 0;
    _offsetPos = 0;
    _absPos = 0;
    _length = 0;
  }

  bool SetMTime(const FILETIME* mTime);
  HRESULT Close();

  UInt64 GetSize() const { return _length; }

  MY_UNKNOWN_IMP1(IOutStream)

  STDMETHOD(Write)(const void* data, UInt32 size, UInt32* processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64* newPosition);
  STDMETHOD(SetSize)(UInt64 newSize);
};

// static NSynchronization::CCriticalSection g_TempPathsCS;

HRESULT COutMultiVolStream::Close() {
  HRESULT res = S_OK;
  FOR_VECTOR(i, Streams) {
    COutFileStream* s = Streams[i].StreamSpec;
    if (s) {
      HRESULT res2 = s->Close();
      if (res2 != S_OK) res = res2;
    }
  }
  return res;
}

bool COutMultiVolStream::SetMTime(const FILETIME* mTime) {
  bool res = true;
  FOR_VECTOR(i, Streams) {
    COutFileStream* s = Streams[i].StreamSpec;
    if (s)
      if (!s->SetMTime(mTime)) res = false;
  }
  return res;
}

STDMETHODIMP COutMultiVolStream::Write(const void* data, UInt32 size, UInt32* processedSize) {
  if (processedSize) *processedSize = 0;
  while (size > 0) {
    if (_streamIndex >= Streams.Size()) {
      CAltStreamInfo altStream;

      FString name;
      name.Add_UInt32(_streamIndex + 1);
      while (name.Len() < 3) name.InsertAtFront(FTEXT('0'));
      name.Insert(0, Prefix);
      altStream.StreamSpec = new COutFileStream;
      altStream.Stream = altStream.StreamSpec;
      if (!altStream.StreamSpec->Create(name, false)) return ::GetLastError();
      {
        // NSynchronization::CCriticalSectionLock lock(g_TempPathsCS);
        TempFiles->Paths.Add(name);
      }

      altStream.Pos = 0;
      altStream.RealSize = 0;
      altStream.Name = name;
      Streams.Add(altStream);
      continue;
    }
    CAltStreamInfo& altStream = Streams[_streamIndex];

    unsigned index = _streamIndex;
    if (index >= Sizes.Size()) index = Sizes.Size() - 1;
    UInt64 volSize = Sizes[index];

    if (_offsetPos >= volSize) {
      _offsetPos -= volSize;
      _streamIndex++;
      continue;
    }
    if (_offsetPos != altStream.Pos) {
      // CMyComPtr<IOutStream> outStream;
      // RINOK(altStream.Stream.QueryInterface(IID_IOutStream, &outStream));
      RINOK(altStream.Stream->Seek(_offsetPos, STREAM_SEEK_SET, NULL));
      altStream.Pos = _offsetPos;
    }

    UInt32 curSize = (UInt32)MyMin((UInt64)size, volSize - altStream.Pos);
    UInt32 realProcessed;
    RINOK(altStream.Stream->Write(data, curSize, &realProcessed));
    data = (void*)((Byte*)data + realProcessed);
    size -= realProcessed;
    altStream.Pos += realProcessed;
    _offsetPos += realProcessed;
    _absPos += realProcessed;
    if (_absPos > _length) _length = _absPos;
    if (_offsetPos > altStream.RealSize) altStream.RealSize = _offsetPos;
    if (processedSize) *processedSize += realProcessed;
    if (altStream.Pos == volSize) {
      _streamIndex++;
      _offsetPos = 0;
    }
    if (realProcessed == 0 && curSize != 0) return E_FAIL;
    break;
  }
  return S_OK;
}

STDMETHODIMP COutMultiVolStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64* newPosition) {
  if (seekOrigin >= 3) return STG_E_INVALIDFUNCTION;
  switch (seekOrigin) {
    case STREAM_SEEK_SET: _absPos = offset; break;
    case STREAM_SEEK_CUR: _absPos += offset; break;
    case STREAM_SEEK_END: _absPos = _length + offset; break;
  }
  _offsetPos = _absPos;
  if (newPosition) *newPosition = _absPos;
  _streamIndex = 0;
  return S_OK;
}

STDMETHODIMP COutMultiVolStream::SetSize(UInt64 newSize) {
  unsigned i = 0;
  while (i < Streams.Size()) {
    CAltStreamInfo& altStream = Streams[i++];
    if ((UInt64)newSize < altStream.RealSize) {
      RINOK(altStream.Stream->SetSize(newSize));
      altStream.RealSize = newSize;
      break;
    }
    newSize -= altStream.RealSize;
  }
  while (i < Streams.Size()) {
    {
      CAltStreamInfo& altStream = Streams.Back();
      altStream.Stream.Release();
      DeleteFileAlways(altStream.Name);
    }
    Streams.DeleteBack();
  }
  _offsetPos = _absPos;
  _streamIndex = 0;
  _length = newSize;
  return S_OK;
}

static const char* const kDefaultArcType = "7z";
static const char* const kDefaultArcExt = "7z";
static const char* const kSFXExtension =
#ifdef _WIN32
    "exe";
#else
    "";
#endif

bool CRenamePair::Prepare() {
  if (RecursedType != NRecursedType::kNonRecursed) return false;
  if (!WildcardParsing) return true;
  return !DoesNameContainWildcard(OldName);
}

extern bool g_CaseSensitive;

static unsigned CompareTwoNames(const wchar_t* s1, const wchar_t* s2) {
  for (unsigned i = 0;; i++) {
    wchar_t c1 = s1[i];
    wchar_t c2 = s2[i];
    if (c1 == 0 || c2 == 0) return i;
    if (c1 == c2) continue;
    if (!g_CaseSensitive && (MyCharUpper(c1) == MyCharUpper(c2))) continue;
    if (IsPathSepar(c1) && IsPathSepar(c2)) continue;
    return i;
  }
}

bool CRenamePair::GetNewPath(bool isFolder, const UString& src, UString& dest) const {
  unsigned num = CompareTwoNames(OldName, src);
  if (OldName[num] == 0) {
    if (src[num] != 0 && !IsPathSepar(src[num]) && num != 0 && !IsPathSepar(src[num - 1]))
      return false;
  } else {
    // OldName[num] != 0
    // OldName = "1\1a.txt"
    // src = "1"

    if (!isFolder || src[num] != 0 || !IsPathSepar(OldName[num]) || OldName[num + 1] != 0)
      return false;
  }
  dest = NewName + src.Ptr(num);
  return true;
}
