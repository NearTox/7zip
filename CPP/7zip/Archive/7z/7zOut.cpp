// 7zOut.cpp

#include "../../../Common/Common.h"

#include "../../../../C/7zCrc.h"

#include "../../../Common/AutoPtr.h"

#include "../../Common/StreamObjects.h"

#include "7zOut.h"

namespace NArchive {
namespace N7z {

static inline unsigned Bv_GetSizeInBytes(const CBoolVector& v) {
  return ((unsigned)v.Size() + 7) / 8;
}

unsigned BoolVector_CountSum(const CBoolVector& v);

// 7-Zip 4.50 - 4.58 contain BUG, so they do not support .7z archives with Unknown field.

void CUInt32DefVector::SetItem(unsigned index, bool defined, UInt32 value) {
  while (index >= Defs.Size()) Defs.Add(false);
  Defs[index] = defined;
  if (!defined) return;
  while (index >= Vals.Size()) Vals.Add(0);
  Vals[index] = value;
}

void CUInt64DefVector::SetItem(unsigned index, bool defined, UInt64 value) {
  while (index >= Defs.Size()) Defs.Add(false);
  Defs[index] = defined;
  if (!defined) return;
  while (index >= Vals.Size()) Vals.Add(0);
  Vals[index] = value;
}

void CArchiveDatabaseOut::AddFile(
    const CFileItem& file, const CFileItem2& file2, const UString& name) {
  unsigned index = Files.Size();
  CTime.SetItem(index, file2.CTimeDefined, file2.CTime);
  ATime.SetItem(index, file2.ATimeDefined, file2.ATime);
  MTime.SetItem(index, file2.MTimeDefined, file2.MTime);
  StartPos.SetItem(index, file2.StartPosDefined, file2.StartPos);
  Attrib.SetItem(index, file2.AttribDefined, file2.Attrib);
  SetItem_Anti(index, file2.IsAnti);
  // SetItem_Aux(index, file2.IsAux);
  Names.Add(name);
  Files.Add(file);
}

}  // namespace N7z
}  // namespace NArchive
