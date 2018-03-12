// Main.cpp

#include "../../../Common/Common.h"
#include "../../../Common/MyWindows.h"
#ifdef _WIN32
#include <Psapi.h>
#endif
#include "../../../../C/CpuArch.h"
#if defined( _7ZIP_LARGE_PAGES)
#include "../../../../C/Alloc.h"
#endif

#include "../../../Common/MyInitGuid.h"
#include "../../../Common/CommandLineParser.h"
#include "../../../Common/IntToString.h"
#include "../../../Common/MyException.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/StringToInt.h"
#include "../../../Common/UTFConvert.h"
#include "../../../Windows/ErrorMsg.h"

#ifdef _WIN32
#include "../../../Windows/MemoryLock.h"
#endif

#include "../../../Windows/TimeUtils.h"

#include "../Common/ArchiveCommandLine.h"
#include "../Common/ExitCode.h"
#include "../Common/Extract.h"

#include "../../Common/RegisterCodec.h"
#include <stdio.h>
#include "../../Common/CreateCoder.h"
#include "../../UI/Common/Property.h"
#include "ExtractCallbackConsole.h"
#include "OpenCallbackConsole.h"
#include "../../MyVersion.h"

using namespace NWindows;
using namespace NFile;
using namespace NCommandLineParser;

extern CStdOutStream *g_StdStream;
extern CStdOutStream *g_ErrStream;

// ---------------------------
// exception messages

static const char *kEverythingIsOk = "Everything is Ok";
static const char *kUserErrorMessage = "Incorrect command line";
static const char *kNoFormats = "7-Zip cannot find the code that works with archives.";
static const char *kUnsupportedArcTypeMessage = "Unsupported archive type";

#ifndef _WIN32
static void GetArguments(int numArgs, const char *args[], UStringVector &parts) {
  parts.Clear();
  for(int i = 0; i < numArgs; i++) {
    UString s = MultiByteToUnicodeString(args[i]);
    parts.Add(s);
  }
}
#endif

static void ThrowException_if_Error(HRESULT res) {
  if(res != S_OK)
    throw CSystemException(res);
}
int _7zExtractorCMD_2(const wchar_t *args) {
  /*#if defined(_WIN32) && !defined(UNDER_CE)
      SetFileApisToOEM();
  #endif*/

  UStringVector commandStrings;
#ifdef useMain
  //NCommandLineParser::SplitCommandLine(GetCommandLineW(), commandStrings);
  //NCommandLineParser::SplitCommandLine(L"t \"T:\\NT.Sync\\Vecttor\\Instalador.Resources\\drivers.7z\"", commandStrings);
  //NCommandLineParser::SplitCommandLine(L"x -o\"C:\\Test\\\" \"T:\\NT.Sync\\Vecttor\\Instalador.Resources\\drivers.7z\"", commandStrings);
#else
  NCommandLineParser::SplitCommandLine(args, commandStrings);
#endif //!useMain

  CArcCmdLineOptions options;
  CArcCmdLineParser parser;
  parser.Parse1(commandStrings, options);
#if defined(_WIN32) && !defined(UNDER_CE)
  NSecurity::EnablePrivilege_SymLink();
#endif

#ifdef _7ZIP_LARGE_PAGES
  SetLargePageSize();
#if defined(_WIN32) && !defined(UNDER_CE)
  NSecurity::EnablePrivilege_LockMemory();
#endif
#endif

  parser.Parse2(options);
  CREATE_CODECS_OBJECT

    ThrowException_if_Error(codecs->Load());
  if(codecs->Formats.Size() == 0) {
    throw kNoFormats;
  }

  CObjectVector<COpenType> types;
  CIntVector excludedFormats;

  int retCode = NExitCode::kSuccess;
  HRESULT hresultMain = S_OK;
  UStringVector ArchivePathsSorted;
  UStringVector ArchivePathsFullSorted;
  CExtractScanConsole scan;
  CDirItemsStat st;

  scan.Init(g_StdStream, g_ErrStream);

  if(g_StdStream) *g_StdStream << "Scanning the drive for archives:" << endl;
  hresultMain = EnumerateDirItemsAndSort(options.arcCensor, ArchivePathsSorted, ArchivePathsFullSorted, st, &scan);

  if(hresultMain == S_OK) {
    scan.PrintStat(st);
    CExtractCallbackConsole *ecs = new CExtractCallbackConsole;
    CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;

    ecs->PasswordIsDefined = options.PasswordEnabled;
    ecs->Password = options.Password;

    ecs->Init(g_StdStream, g_ErrStream);
    ecs->MultiArcMode = (ArchivePathsSorted.Size() > 1);

    CExtractOptions eo = options.ExtractOptions;
    eo.TestMode = options.IsTestCommand();

    UString errorMessage;
    CDecompressStat stat;
    hresultMain = Extract(codecs, types, excludedFormats, ArchivePathsSorted, ArchivePathsFullSorted, options.Censor.Pairs.Front().Head, eo, ecs, ecs, errorMessage, stat);

    if(!errorMessage.IsEmpty()) {
      if(g_ErrStream)
        *g_ErrStream << endl << "ERROR:" << endl << errorMessage << endl;
      if(hresultMain == S_OK)
        hresultMain = E_FAIL;
    }
    bool isError = false;

    if(g_StdStream) {
      *g_StdStream << endl;

      if(ecs->NumTryArcs > 1) {
        *g_StdStream << "Archives: " << ecs->NumTryArcs << endl;
        *g_StdStream << "OK archives: " << ecs->NumOkArcs << endl;
      }
    }

    if(ecs->NumCantOpenArcs != 0) {
      isError = true;
      if(g_StdStream)
        *g_StdStream << "Can't open as archive: " << ecs->NumCantOpenArcs << endl;
    }

    if(ecs->NumArcsWithError != 0) {
      isError = true;
      if(g_StdStream)
        *g_StdStream << "Archives with Errors: " << ecs->NumArcsWithError << endl;
    }

    if(g_StdStream) {
      if(ecs->NumArcsWithWarnings != 0)
        *g_StdStream << "Archives with Warnings: " << ecs->NumArcsWithWarnings << endl;

      if(ecs->NumOpenArcWarnings != 0) {
        *g_StdStream << endl;
        if(ecs->NumOpenArcWarnings != 0)
          *g_StdStream << "Warnings: " << ecs->NumOpenArcWarnings << endl;
      }
    }

    if(ecs->NumOpenArcErrors != 0) {
      isError = true;
      if(g_StdStream) {
        *g_StdStream << endl;
        if(ecs->NumOpenArcErrors != 0)
          *g_StdStream << "Open Errors: " << ecs->NumOpenArcErrors << endl;
      }
    }

    if(isError)
      retCode = NExitCode::kFatalError;

    if(g_StdStream)
      if(ecs->NumArcsWithError != 0 || ecs->NumFileErrors != 0) {
        // if (ecs->NumArchives > 1)
        {
          *g_StdStream << endl;
          if(ecs->NumFileErrors != 0)
            *g_StdStream << "Sub items Errors: " << ecs->NumFileErrors << endl;
        }
      } else if(hresultMain == S_OK) {
        if(stat.NumFolders != 0)
          *g_StdStream << "Folders: " << stat.NumFolders << endl;
        if(stat.NumFiles != 1 || stat.NumFolders != 0 || stat.NumAltStreams != 0)
          *g_StdStream << "Files: " << stat.NumFiles << endl;
        if(stat.NumAltStreams != 0) {
          *g_StdStream << "Alternate Streams: " << stat.NumAltStreams << endl;
          *g_StdStream << "Alternate Streams Size: " << stat.AltStreams_UnpackSize << endl;
        }

        *g_StdStream
          << "Size:       " << stat.UnpackSize << endl
          << "Compressed: " << stat.PackSize << endl;
      }
  }
  ThrowException_if_Error(hresultMain);

  return retCode;
}