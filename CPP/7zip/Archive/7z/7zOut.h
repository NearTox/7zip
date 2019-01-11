// 7zOut.h

#ifndef __7Z_OUT_H
#define __7Z_OUT_H

#include "7zCompressionMode.h"
#include "7zHeader.h"
#include "7zItem.h"

#include "../../Common/OutBuffer.h"
#include "../../Common/StreamUtils.h"

namespace NArchive {
namespace N7z {

class CWriteBufferLoc {
  Byte* _data;
  size_t _size;
  size_t _pos;

 public:
  CWriteBufferLoc() : _size(0), _pos(0) {}
  void Init(Byte* data, size_t size) {
    _data = data;
    _size = size;
    _pos = 0;
  }
  void WriteBytes(const void* data, size_t size) {
    if (size == 0) return;
    if (size > _size - _pos) throw 1;
    memcpy(_data + _pos, data, size);
    _pos += size;
  }
  void WriteByte(Byte b) {
    if (_size == _pos) throw 1;
    _data[_pos++] = b;
  }
  size_t GetPos() const { return _pos; }
};

struct CHeaderOptions {
  bool CompressMainHeader;
  /*
  bool WriteCTime;
  bool WriteATime;
  bool WriteMTime;
  */

  CHeaderOptions() :
      CompressMainHeader(true)
  /*
  , WriteCTime(false)
  , WriteATime(false)
  , WriteMTime(true)
  */
  {}
};

struct CFileItem2 {
  UInt64 CTime;
  UInt64 ATime;
  UInt64 MTime;
  UInt64 StartPos;
  UInt32 Attrib;

  bool CTimeDefined;
  bool ATimeDefined;
  bool MTimeDefined;
  bool StartPosDefined;
  bool AttribDefined;
  bool IsAnti;
  // bool IsAux;

  /*
  void Init()
  {
    CTimeDefined = false;
    ATimeDefined = false;
    MTimeDefined = false;
    StartPosDefined = false;
    AttribDefined = false;
    IsAnti = false;
    // IsAux = false;
  }
  */
};

struct COutFolders {
  CUInt32DefVector FolderUnpackCRCs;  // Now we use it for headers only.

  CRecordVector<CNum> NumUnpackStreamsVector;
  CRecordVector<UInt64> CoderUnpackSizes;  // including unpack sizes of bond coders

  void OutFoldersClear() {
    FolderUnpackCRCs.Clear();
    NumUnpackStreamsVector.Clear();
    CoderUnpackSizes.Clear();
  }

  void OutFoldersReserveDown() {
    FolderUnpackCRCs.ReserveDown();
    NumUnpackStreamsVector.ReserveDown();
    CoderUnpackSizes.ReserveDown();
  }
};

struct CArchiveDatabaseOut : public COutFolders {
  CRecordVector<UInt64> PackSizes;
  CUInt32DefVector PackCRCs;
  CObjectVector<CFolder> Folders;

  CRecordVector<CFileItem> Files;
  UStringVector Names;
  CUInt64DefVector CTime;
  CUInt64DefVector ATime;
  CUInt64DefVector MTime;
  CUInt64DefVector StartPos;
  CUInt32DefVector Attrib;
  CBoolVector IsAnti;

  /*
  CBoolVector IsAux;

  CByteBuffer SecureBuf;
  CRecordVector<UInt32> SecureSizes;
  CRecordVector<UInt32> SecureIDs;

  void ClearSecure()
  {
    SecureBuf.Free();
    SecureSizes.Clear();
    SecureIDs.Clear();
  }
  */

  void Clear() {
    OutFoldersClear();

    PackSizes.Clear();
    PackCRCs.Clear();
    Folders.Clear();

    Files.Clear();
    Names.Clear();
    CTime.Clear();
    ATime.Clear();
    MTime.Clear();
    StartPos.Clear();
    Attrib.Clear();
    IsAnti.Clear();

    /*
    IsAux.Clear();
    ClearSecure();
    */
  }

  void ReserveDown() {
    OutFoldersReserveDown();

    PackSizes.ReserveDown();
    PackCRCs.ReserveDown();
    Folders.ReserveDown();

    Files.ReserveDown();
    Names.ReserveDown();
    CTime.ReserveDown();
    ATime.ReserveDown();
    MTime.ReserveDown();
    StartPos.ReserveDown();
    Attrib.ReserveDown();
    IsAnti.ReserveDown();

    /*
    IsAux.ReserveDown();
    */
  }

  bool IsEmpty() const {
    return (
        PackSizes.IsEmpty() && NumUnpackStreamsVector.IsEmpty() && Folders.IsEmpty() &&
        Files.IsEmpty());
  }

  bool CheckNumFiles() const {
    unsigned size = Files.Size();
    return (
        CTime.CheckSize(size) && ATime.CheckSize(size) && MTime.CheckSize(size) &&
        StartPos.CheckSize(size) && Attrib.CheckSize(size) &&
        (size == IsAnti.Size() || IsAnti.Size() == 0));
  }

  bool IsItemAnti(unsigned index) const { return (index < IsAnti.Size() && IsAnti[index]); }
  // bool IsItemAux(unsigned index) const { return (index < IsAux.Size() && IsAux[index]); }

  void SetItem_Anti(unsigned index, bool isAnti) {
    while (index >= IsAnti.Size()) IsAnti.Add(false);
    IsAnti[index] = isAnti;
  }
  /*
  void SetItem_Aux(unsigned index, bool isAux)
  {
    while (index >= IsAux.Size())
      IsAux.Add(false);
    IsAux[index] = isAux;
  }
  */

  void AddFile(const CFileItem& file, const CFileItem2& file2, const UString& name);
};

}  // namespace N7z
}  // namespace NArchive

#endif
