// Extract.cpp

#include "../../../Common/Common.h"

#include "../../../../C/Sort.h"

#include "../../../Common/StringConvert.h"

#include "../../../Windows/FileDir.h"
#include "../../../Windows/PropVariant.h"
#include "../../../Windows/PropVariantConv.h"

#include "../Common/ExtractingFilePath.h"

#include "Extract.h"
#include "SetProperties.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;

static HRESULT DecompressArchive(CCodecs *codecs, const CArchiveLink &arcLink, UInt64 packSize, const NWildcard::CCensorNode &wildcardCensor, const CExtractOptions &options, bool calcCrc, IExtractCallbackUI *callback, CArchiveExtractCallback *ecs, UString &errorMessage, UInt64 &stdInProcessed) {
  const CArc &arc = arcLink.Arcs.Back();
  stdInProcessed = 0;
  IInArchive *archive = arc.Archive;
  CRecordVector<UInt32> realIndices;

  UStringVector removePathParts;

  FString outDir = options.OutputDir;
  UString replaceName = arc.DefaultName;

  if(arcLink.Arcs.Size() > 1) {
    // Most "pe" archives have same name of archive subfile "[0]" or ".rsrc_1".
    // So it extracts different archives to one folder.
    // We will use top level archive name
    const CArc &arc0 = arcLink.Arcs[0];
    if(StringsAreEqualNoCase_Ascii(codecs->Formats[arc0.FormatIndex].Name, "pe"))
      replaceName = arc0.DefaultName;
  }

  outDir.Replace(FSTRING_ANY_MASK, us2fs(Get_Correct_FsFile_Name(replaceName)));

  bool elimIsPossible = false;
  UString elimPrefix; // only pure name without dir delimiter
  FString outDirReduced = outDir;
  bool allFilesAreAllowed = wildcardCensor.AreAllAllowed();

  UInt32 numItems;
  RINOK(archive->GetNumberOfItems(&numItems));

  CReadArcItem item;

  for(UInt32 i = 0; i < numItems; i++) {
    if(elimIsPossible || !allFilesAreAllowed) {
      RINOK(arc.GetItem(i, item));
    } else {
#ifdef SUPPORT_ALT_STREAMS
      item.IsAltStream = false;
#endif
    }
    if(elimIsPossible) {
      const UString &s =
#ifdef SUPPORT_ALT_STREAMS
        item.MainPath;
#else
        item.Path;
#endif
      if(!IsPath1PrefixedByPath2(s, elimPrefix))
        elimIsPossible = false;
      else {
        wchar_t c = s[elimPrefix.Len()];
        if(c == 0) {
          if(!item.MainIsDir)
            elimIsPossible = false;
        } else if(!IsPathSepar(c))
          elimIsPossible = false;
      }
    }

    if(!allFilesAreAllowed) {
      if(!CensorNode_CheckPath(wildcardCensor, item))
        continue;
    }

    realIndices.Add(i);
  }

  if(realIndices.Size() == 0) {
    callback->ThereAreNoFiles();
    return callback->ExtractResult(S_OK);
  }

  if(elimIsPossible) {
    removePathParts.Add(elimPrefix);
    // outDir = outDirReduced;
  }

#ifdef _WIN32
  // GetCorrectFullFsPath doesn't like "..".
  // outDir.TrimRight();
  // outDir = GetCorrectFullFsPath(outDir);
#endif

  if(outDir.IsEmpty())
    outDir = FTEXT(".") FSTRING_PATH_SEPARATOR;
  /*
  #ifdef _WIN32
  else if (NName::IsAltPathPrefix(outDir)) {}
  #endif
  */
  else if(!CreateComplexDir(outDir)) {
    HRESULT res = ::GetLastError();
    if(res == S_OK)
      res = E_FAIL;
    errorMessage.SetFromAscii("Can not create output directory: ");
    errorMessage += fs2us(outDir);
    return res;
  }

  ecs->Init(nullptr, &arc, callback, false, options.TestMode, outDir, removePathParts, false, packSize);

#ifdef SUPPORT_LINKS

  if(!options.TestMode) {
    RINOK(ecs->PrepareHardLinks(&realIndices));
  }

#endif

  HRESULT result;
  Int32 testMode = (options.TestMode && !calcCrc) ? 1 : 0;
  result = archive->Extract(&realIndices.Front(), realIndices.Size(), testMode, ecs);
  if(result == S_OK)
    result = ecs->SetDirsTimes();
  return callback->ExtractResult(result);
}

/* v9.31: BUG was fixed:
   Sorted list for file paths was sorted with case insensitive compare function.
   But FindInSorted function did binary search via case sensitive compare function */

int Find_FileName_InSortedVector(const UStringVector &fileName, const UString &name) {
  unsigned left = 0, right = fileName.Size();
  while(left != right) {
    unsigned mid = (left + right) / 2;
    const UString &midValue = fileName[mid];
    int compare = CompareFileNames(name, midValue);
    if(compare == 0)
      return mid;
    if(compare < 0)
      right = mid;
    else
      left = mid + 1;
  }
  return -1;
}

HRESULT Extract(CCodecs *codecs, const CObjectVector<COpenType> &types, const CIntVector &excludedFormats, UStringVector &arcPaths, UStringVector &arcPathsFull, const NWildcard::CCensorNode &wildcardCensor, const CExtractOptions &options, IOpenCallbackUI *openCallback, IExtractCallbackUI *extractCallback, UString &errorMessage, CDecompressStat &st) {
  st.Clear();
  UInt64 totalPackSize = 0;
  CRecordVector<UInt64> arcSizes;
  unsigned numArcs = arcPaths.Size();
  unsigned i;
  for(i = 0; i < numArcs; i++) {
    NFind::CFileInfo fi;
    fi.Size = 0;
    const FString &arcPath = us2fs(arcPaths[i]);
    if(!fi.Find(arcPath))
      throw "there is no such archive";
    if(fi.IsDir())
      throw "can't decompress folder";
    arcSizes.Add(fi.Size);
    totalPackSize += fi.Size;
  }

  CBoolArr skipArcs(numArcs);
  for(i = 0; i < numArcs; i++)
    skipArcs[i] = false;

  CArchiveExtractCallback *ecs = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> ec(ecs);
  bool multi = (numArcs > 1);
  ecs->InitForMulti(multi);
#ifndef _SFX
  //ecs->SetHashMethods(nullptr);
#endif

  if(multi) {
    RINOK(extractCallback->SetTotal(totalPackSize));
  }

  UInt64 totalPackProcessed = 0;
  bool thereAreNotOpenArcs = false;

  for(i = 0; i < numArcs; i++) {
    if(skipArcs[i])
      continue;

    const UString &arcPath = arcPaths[i];
    NFind::CFileInfo fi;
    if(!fi.Find(us2fs(arcPath)) || fi.IsDir())
      throw "there is no such archive";

    RINOK(extractCallback->BeforeOpen(arcPath, options.TestMode));
    CArchiveLink arcLink;

    CObjectVector<COpenType> types2 = types;

    COpenOptions op;
    op.codecs = codecs;
    op.types = &types2;
    op.excludedFormats = &excludedFormats;
    op.stream = nullptr;
    op.filePath = arcPath;

    HRESULT result = arcLink.Open3(op, openCallback);

    if(result == E_ABORT)
      return result;

    if(result == S_OK && arcLink.NonOpen_ErrorInfo.ErrorFormatIndex >= 0)
      result = S_FALSE;

    // arcLink.Set_ErrorsText();
    RINOK(extractCallback->OpenResult(codecs, arcLink, arcPath, result));

    if(result != S_OK) {
      thereAreNotOpenArcs = true;
      NFind::CFileInfo fi;
      if(fi.Find(us2fs(arcPath)))
        if(!fi.IsDir())
          totalPackProcessed += fi.Size;
      continue;
    }

    // numVolumes += arcLink.VolumePaths.Size();
    // arcLink.VolumesSize;

    // totalPackSize -= DeleteUsedFileNamesFromList(arcLink, i + 1, arcPaths, arcPathsFull, &arcSizes);
    // numArcs = arcPaths.Size();
    if(arcLink.VolumePaths.Size() != 0) {
      Int64 correctionSize = arcLink.VolumesSize;
      FOR_VECTOR(v, arcLink.VolumePaths) {
        int index = Find_FileName_InSortedVector(arcPathsFull, arcLink.VolumePaths[v]);
        if(index >= 0) {
          if((unsigned)index > i) {
            skipArcs[(unsigned)index] = true;
            correctionSize -= arcSizes[(unsigned)index];
          }
        }
      }
      if(correctionSize != 0) {
        Int64 newPackSize = (Int64)totalPackSize + correctionSize;
        if(newPackSize < 0)
          newPackSize = 0;
        totalPackSize = newPackSize;
        RINOK(extractCallback->SetTotal(totalPackSize));
      }
    }

    CArc &arc = arcLink.Arcs.Back();
    arc.MTimeDefined = !fi.IsDevice;
    arc.MTime = fi.MTime;

    UInt64 packProcessed;

    RINOK(DecompressArchive(codecs, arcLink, fi.Size + arcLink.VolumesSize, wildcardCensor, options, false, extractCallback, ecs, errorMessage, packProcessed));

    packProcessed = fi.Size + arcLink.VolumesSize;
    totalPackProcessed += packProcessed;
    ecs->LocalProgressSpec->InSize += packProcessed;
    ecs->LocalProgressSpec->OutSize = ecs->UnpackSize;
    if(!errorMessage.IsEmpty())
      return E_FAIL;
  }

  if(multi || thereAreNotOpenArcs) {
    RINOK(extractCallback->SetTotal(totalPackSize));
    RINOK(extractCallback->SetCompleted(&totalPackProcessed));
  }

  st.NumFolders = ecs->NumFolders;
  st.NumFiles = ecs->NumFiles;
  st.NumAltStreams = ecs->NumAltStreams;
  st.UnpackSize = ecs->UnpackSize;
  st.AltStreams_UnpackSize = ecs->AltStreams_UnpackSize;
  st.NumArchives = arcPaths.Size();
  st.PackSize = ecs->LocalProgressSpec->InSize;
  return S_OK;
}