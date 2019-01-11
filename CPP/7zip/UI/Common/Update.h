// Update.h

#ifndef __COMMON_UPDATE_H
#define __COMMON_UPDATE_H

#include "../../../Common/Wildcard.h"

#include "ArchiveOpenCallback.h"
#include "LoadCodecs.h"
#include "OpenArchive.h"
#include "Property.h"

#include "DirItem.h"

enum EArcNameMode {
  k_ArcNameMode_Smart,
  k_ArcNameMode_Exact,
  k_ArcNameMode_Add,
};

struct CCompressionMethodMode {
  bool Type_Defined;
  COpenType Type;
  CObjectVector<CProperty> Properties;

  CCompressionMethodMode() : Type_Defined(false) {}
};

namespace NRecursedType {
enum EEnum { kRecursed, kWildcardOnlyRecursed, kNonRecursed };
}

struct CRenamePair {
  UString OldName;
  UString NewName;
  bool WildcardParsing;
  NRecursedType::EEnum RecursedType;

  CRenamePair() : WildcardParsing(true), RecursedType(NRecursedType::kNonRecursed) {}

  bool Prepare();
  bool GetNewPath(bool isFolder, const UString& src, UString& dest) const;
};

#endif
