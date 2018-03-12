// Extract.h

#ifndef __EXTRACT_H
#define __EXTRACT_H

#include "../../../Windows/FileFind.h"
#include "../../Archive/IArchive.h"
#include "ArchiveExtractCallback.h"
#include "ArchiveOpenCallback.h"
#include "Property.h"
#include "../Common/LoadCodecs.h"
struct CExtractOptions {
  FString OutputDir;
  bool TestMode;
  CExtractOptions() : TestMode(false) {}
};

struct CDecompressStat {
  UInt64 NumArchives;
  UInt64 UnpackSize;
  UInt64 AltStreams_UnpackSize;
  UInt64 PackSize;
  UInt64 NumFolders;
  UInt64 NumFiles;
  UInt64 NumAltStreams;

  void Clear() {
    NumArchives = UnpackSize = AltStreams_UnpackSize = PackSize = NumFolders = NumFiles = NumAltStreams = 0;
  }
};

HRESULT Extract(CCodecs *codecs, const CObjectVector<COpenType> &types, const CIntVector &excludedFormats, UStringVector &archivePaths, UStringVector &archivePathsFull, const NWildcard::CCensorNode &wildcardCensor, const CExtractOptions &options, IOpenCallbackUI *openCallback, IExtractCallbackUI *extractCallback, UString &errorMessage, CDecompressStat &st);

#endif
