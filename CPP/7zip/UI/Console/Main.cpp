// Main.cpp

#include "../../../Common/Common.h"

#include "../../../Common/MyWindows.h"

#ifdef _WIN32
#  include <Psapi.h>
#endif

#include "../../../../C/CpuArch.h"

#include "../../../Common/MyInitGuid.h"

#include "../../../Common/CommandLineParser.h"
#include "../../../Common/IntToString.h"
#include "../../../Common/MyException.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/StringToInt.h"
#include "../../../Common/UTFConvert.h"

#include "../../../Windows/ErrorMsg.h"

#include "../../../Windows/TimeUtils.h"

#include "../Common/ArchiveCommandLine.h"
#include "../Common/Bench.h"
#include "../Common/ExitCode.h"
#include "../Common/Extract.h"

#include "../../Common/RegisterCodec.h"

#include "ConsoleClose.h"
#include "ExtractCallbackConsole.h"
#include "List.h"
#include "OpenCallbackConsole.h"
#include "UpdateCallbackConsole.h"

#include "HashCon.h"

#ifdef PROG_VARIANT_R
#  include "../../../../C/7zVersion.h"
#else
#  include "../../MyVersion.h"
#endif

using namespace NWindows;
using namespace NFile;
using namespace NCommandLineParser;

#ifdef _WIN32
HINSTANCE g_hInstance = 0;
#endif

extern CStdOutStream* g_StdStream;
extern CStdOutStream* g_ErrStream;

extern unsigned g_NumCodecs;
extern const CCodecInfo* g_Codecs[];

extern unsigned g_NumHashers;
extern const CHasherInfo* g_Hashers[];

static const char* const kCopyrightString = "\n7-Zip"
#ifdef PROG_VARIANT_R
                                            " (r)"
#else
                                            " (a)"
#endif

                                            " " MY_VERSION_CPU " : " MY_COPYRIGHT_DATE "\n\n";

static const char* const kHelpString =
    "Usage: 7z"
#ifdef PROG_VARIANT_R
    "r"
#else
    "a"
#endif
    " [<switches>...] <archive_name> [<file_names>...]\n"
    "\n"
    "eXtract files with full paths\n"
    "\n"
    "<Switches>\n"
    "  -- : Stop switches parsing\n"
    // TODO: Is this functional?
    "  @listfile : set path to listfile that contains file names\n"
    "  -o{Directory} : set Output directory\n"
#ifndef _NO_CRYPTO
    "  -p{Password} : set Password\n"
#endif
    ;

// ---------------------------
// exception messages

static const char* const kEverythingIsOk = "Everything is Ok";
static const char* const kUserErrorMessage = "Incorrect command line";
static const char* const kNoFormats = "7-Zip cannot find the code that works with archives.";
static const char* const kUnsupportedArcTypeMessage = "Unsupported archive type";
// static const char * const kUnsupportedUpdateArcType = "Can't create archive for that type";

#define kDefaultSfxModule "7zCon.sfx"

#ifndef _WIN32
static void GetArguments(int numArgs, const char* args[], UStringVector& parts) {
  parts.Clear();
  for (int i = 0; i < numArgs; i++) {
    UString s = MultiByteToUnicodeString(args[i]);
    parts.Add(s);
  }
}
#endif

static void ShowCopyrightAndHelp(CStdOutStream* so, bool needHelp) {
  if (!so) return;
  *so << kCopyrightString;
  // *so << "# CPUs: " << (UInt64)NWindows::NSystem::GetNumberOfProcessors() << endl;
  if (needHelp) *so << kHelpString;
}

static inline char GetHex(unsigned val) {
  return (char)((val < 10) ? ('0' + val) : ('A' + (val - 10)));
}

static void ThrowException_if_Error(HRESULT res) {
  if (res != S_OK) throw CSystemException(res);
}

#ifndef UNDER_CE

#  define SHIFT_SIZE_VALUE(x, num) (((x) + (1 << (num)) - 1) >> (num))

EXTERN_C_BEGIN
typedef BOOL(WINAPI* Func_GetProcessMemoryInfo)(
    HANDLE Process, PPROCESS_MEMORY_COUNTERS ppsmemCounters, DWORD cb);
typedef BOOL(WINAPI* Func_QueryProcessCycleTime)(HANDLE Process, PULONG64 CycleTime);
EXTERN_C_END

#endif

static inline UInt64 GetTime64(const FILETIME& t) {
  return ((UInt64)t.dwHighDateTime << 32) | t.dwLowDateTime;
}

int Main2(
#ifndef _WIN32
    int numArgs, char* args[]
#endif
) {
#if defined(_WIN32) && !defined(UNDER_CE)
  SetFileApisToOEM();
#endif

  UStringVector commandStrings;

#ifdef _WIN32
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), commandStrings);
#else
  GetArguments(numArgs, args, commandStrings);
#endif

#ifndef UNDER_CE
  if (commandStrings.Size() > 0) commandStrings.Delete(0);
#endif

  if (commandStrings.Size() == 0) {
    ShowCopyrightAndHelp(g_StdStream, true);
    return 0;
  }

  CArcCmdLineOptions options;

  CArcCmdLineParser parser;

  parser.Parse1(commandStrings, options);

  g_StdOut.IsTerminalMode = options.IsStdOutTerminal;
  g_StdErr.IsTerminalMode = options.IsStdErrTerminal;

  if (options.Number_for_Out != k_OutStream_stdout)
    g_StdStream = (options.Number_for_Out == k_OutStream_stderr ? &g_StdErr : nullptr);

  if (options.Number_for_Errors != k_OutStream_stderr)
    g_ErrStream = (options.Number_for_Errors == k_OutStream_stdout ? &g_StdOut : nullptr);

  CStdOutStream* percentsStream = nullptr;
  if (options.Number_for_Percents != k_OutStream_disabled)
    percentsStream = (options.Number_for_Percents == k_OutStream_stderr) ? &g_StdErr : &g_StdOut;
  ;

  ShowCopyrightAndHelp(g_StdStream, false);

  parser.Parse2(options);

  unsigned percentsNameLevel = 1;
  if (options.LogLevel == 0 || options.Number_for_Percents != options.Number_for_Out)
    percentsNameLevel = 2;

  unsigned consoleWidth = 80;

  if (percentsStream) {
#ifdef _WIN32

#  if !defined(UNDER_CE)
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo))
      consoleWidth = consoleInfo.dwSize.X;
#  endif

#else

    struct winsize w;
    if (ioctl(0, TIOCGWINSZ, &w) ==) consoleWidth = w.ws_col;

#endif
  }

  CREATE_CODECS_OBJECT

  codecs->CaseSensitiveChange = options.CaseSensitiveChange;
  codecs->CaseSensitive = options.CaseSensitive;
  ThrowException_if_Error(codecs->Load());

  if (codecs->Formats.Size() == 0) { throw kNoFormats; }

  CObjectVector<COpenType> types;
  if (!ParseOpenTypes(*codecs, options.ArcType, types)) throw kUnsupportedArcTypeMessage;

  CIntVector excludedFormats;
  FOR_VECTOR(k, options.ExcludedArcTypes) {
    CIntVector tempIndices;
    if (!codecs->FindFormatForArchiveType(options.ExcludedArcTypes[k], tempIndices) ||
        tempIndices.Size() != 1)
      throw kUnsupportedArcTypeMessage;
    excludedFormats.AddToUniqueSorted(tempIndices[0]);
    // excludedFormats.Sort();
  }

  int retCode = NExitCode::kSuccess;
  HRESULT hresultMain = S_OK;

  // bool showStat = options.ShowTime;

  /*
  if (!options.EnableHeaders ||
      options.TechMode)
    showStat = false;
  */

  UStringVector ArchivePathsSorted;
  UStringVector ArchivePathsFullSorted;
  {
    CExtractScanConsole scan;

    scan.Init(g_StdStream, g_ErrStream, percentsStream);
    scan.SetWindowWidth(consoleWidth);

    if (g_StdStream) *g_StdStream << "Scanning the drive for archives:" << endl;

    CDirItemsStat st;

    scan.StartScanning();

    hresultMain = EnumerateDirItemsAndSort(
        options.arcCensor, NWildcard::k_RelatPath,
        UString(),  // addPathPrefix
        ArchivePathsSorted, ArchivePathsFullSorted, st, &scan);

    scan.CloseScanning();

    if (hresultMain == S_OK) {
      scan.PrintStat(st);
    } else {
      /*
      if (res != E_ABORT)
      {
        throw CSystemException(res);
        // errorInfo.Message = "Scanning error";
      }
      return res;
      */
    }
  }

  if (hresultMain == S_OK) {
    CExtractCallbackConsole* ecs = new CExtractCallbackConsole;
    CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;

#ifndef _NO_CRYPTO
    ecs->PasswordIsDefined = options.PasswordEnabled;
    ecs->Password = options.Password;
#endif

    ecs->Init(g_StdStream, g_ErrStream, percentsStream);
    ecs->MultiArcMode = (ArchivePathsSorted.Size() > 1);

    ecs->LogLevel = options.LogLevel;
    ecs->PercentsNameLevel = percentsNameLevel;

    if (percentsStream) ecs->SetWindowWidth(consoleWidth);

    /*
    COpenCallbackConsole openCallback;
    openCallback.Init(g_StdStream, g_ErrStream);

    #ifndef _NO_CRYPTO
    openCallback.PasswordIsDefined = options.PasswordEnabled;
    openCallback.Password = options.Password;
    #endif
    */

    CExtractOptions eo;
    (CExtractOptionsBase&)eo = options.ExtractOptions;

#ifndef _SFX
    eo.Properties = options.Properties;
#endif

    UString errorMessage;
    CDecompressStat stat;
    CHashBundle hb;
    IHashCalc* hashCalc = nullptr;

    if (!options.HashMethods.IsEmpty()) {
      hashCalc = &hb;
      ThrowException_if_Error(hb.SetMethods(options.HashMethods));
      // hb.Init();
    }

    hresultMain = Extract(
        codecs, types, excludedFormats, ArchivePathsSorted, ArchivePathsFullSorted,
        options.Censor.Pairs.Front().Head, eo, ecs, ecs, hashCalc, errorMessage, stat);

    ecs->ClosePercents();

    if (!errorMessage.IsEmpty()) {
      if (g_ErrStream) *g_ErrStream << endl << "ERROR:" << endl << errorMessage << endl;
      if (hresultMain == S_OK) hresultMain = E_FAIL;
    }

    CStdOutStream* so = g_StdStream;

    bool isError = false;

    if (so) {
      *so << endl;

      if (ecs->NumTryArcs > 1) {
        *so << "Archives: " << ecs->NumTryArcs << endl;
        *so << "OK archives: " << ecs->NumOkArcs << endl;
      }
    }

    if (ecs->NumCantOpenArcs != 0) {
      isError = true;
      if (so) *so << "Can't open as archive: " << ecs->NumCantOpenArcs << endl;
    }

    if (ecs->NumArcsWithError != 0) {
      isError = true;
      if (so) *so << "Archives with Errors: " << ecs->NumArcsWithError << endl;
    }

    if (so) {
      if (ecs->NumArcsWithWarnings != 0)
        *so << "Archives with Warnings: " << ecs->NumArcsWithWarnings << endl;

      if (ecs->NumOpenArcWarnings != 0) {
        *so << endl;
        if (ecs->NumOpenArcWarnings != 0) *so << "Warnings: " << ecs->NumOpenArcWarnings << endl;
      }
    }

    if (ecs->NumOpenArcErrors != 0) {
      isError = true;
      if (so) {
        *so << endl;
        if (ecs->NumOpenArcErrors != 0) *so << "Open Errors: " << ecs->NumOpenArcErrors << endl;
      }
    }

    if (isError) retCode = NExitCode::kFatalError;

    if (so)
      if (ecs->NumArcsWithError != 0 || ecs->NumFileErrors != 0) {
        // if (ecs->NumArchives > 1)
        {
          *so << endl;
          if (ecs->NumFileErrors != 0) *so << "Sub items Errors: " << ecs->NumFileErrors << endl;
        }
      } else if (hresultMain == S_OK) {
        if (stat.NumFolders != 0) *so << "Folders: " << stat.NumFolders << endl;
        if (stat.NumFiles != 1 || stat.NumFolders != 0 || stat.NumAltStreams != 0)
          *so << "Files: " << stat.NumFiles << endl;
        if (stat.NumAltStreams != 0) {
          *so << "Alternate Streams: " << stat.NumAltStreams << endl;
          *so << "Alternate Streams Size: " << stat.AltStreams_UnpackSize << endl;
        }

        *so << "Size:       " << stat.UnpackSize << endl << "Compressed: " << stat.PackSize << endl;
        if (hashCalc) {
          *so << endl;
          PrintHashStat(*so, hb);
        }
      }
  }

  ThrowException_if_Error(hresultMain);

  return retCode;
}
