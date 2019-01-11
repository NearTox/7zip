// ArchiveCommandLine.cpp

#include "../../../Common/Common.h"
#undef printf
#undef sprintf

#ifdef _WIN32
#  ifndef UNDER_CE
#    include <io.h>
#  endif
#else
// for isatty()
#  include <unistd.h>
#endif

#include <stdio.h>

#ifdef _7ZIP_LARGE_PAGES
#  include "../../../../C/Alloc.h"
#endif

#include "../../../Common/ListFileUtils.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/StringToInt.h"

#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileName.h"
#ifdef _WIN32
#  include "../../../Windows/FileMapping.h"
#  include "../../../Windows/MemoryLock.h"
#  include "../../../Windows/Synchronization.h"
#endif

#include "ArchiveCommandLine.h"
#include "EnumDirItems.h"
#include "Update.h"

extern bool g_CaseSensitive;
extern bool g_PathTrailReplaceMode;

#ifdef _7ZIP_LARGE_PAGES
bool g_LargePagesMode = false;
#endif

#ifdef UNDER_CE

#  define MY_IS_TERMINAL(x) false;

#else

#  if _MSC_VER >= 1400
#    define MY_isatty_fileno(x) _isatty(_fileno(x))
#  else
#    define MY_isatty_fileno(x) isatty(fileno(x))
#  endif

#  define MY_IS_TERMINAL(x) (MY_isatty_fileno(x) != 0);

#endif

using namespace NCommandLineParser;
using namespace NWindows;
using namespace NFile;

static bool StringToUInt32(const wchar_t* s, UInt32& v) {
  if (*s == 0) return false;
  const wchar_t* end;
  v = ConvertStringToUInt32(s, &end);
  return *end == 0;
}

int g_CodePage = -1;

namespace NKey {
enum Enum {
  kOutputDir,
#ifndef _NO_CRYPTO
  kPassword,
#endif
};

}  // namespace NKey

namespace NRecursedPostCharIndex {
enum EEnum { kWildcardRecursionOnly = 0, kNoRecursion = 1 };
}

static const char kImmediateNameID = '!';
static const char kMapNameID = '#';
static const char kFileListID = '@';

static const char kSomeCludePostStringMinSize = 2;               // at least <@|!><N>ame must be
static const char kSomeCludeAfterRecursedPostStringMinSize = 2;  // at least <@|!><N>ame must be

static const char* const kOverwritePostCharSet = "asut";

static const CSwitchForm kSwitchForms[] = {
    {"o", NSwitchType::kString, false, 1},
#ifndef _NO_CRYPTO
    {"p", NSwitchType::kString},
#endif
};

static const char* const kUniversalWildcard = "*";
static const unsigned kMinNonSwitchWords = 1;
static const unsigned kCommandIndex = 0;

// static const char * const kUserErrorMessage  = "Incorrect command line";
static const char* const kCannotFindListFile = "Cannot find listfile";
static const char* const kIncorrectListFile =
    "Incorrect item in listfile.\nCheck charset encoding and -scs switch.";
static const char* const kTerminalOutError = "I won't write compressed data to a terminal";
static const char* const kSameTerminalError =
    "I won't write data and program's messages to same stream";
static const char* const kEmptyFilePath = "Empty file path";

// ------------------------------------------------------------------
// filenames functions

static void AddNameToCensor(
    NWildcard::CCensor& censor, const UString& name, bool include, NRecursedType::EEnum type,
    bool wildcardMatching) {
  bool recursed = false;

  switch (type) {
    case NRecursedType::kWildcardOnlyRecursed: recursed = DoesNameContainWildcard(name); break;
    case NRecursedType::kRecursed: recursed = true; break;
  }
  censor.AddPreItem(include, name, recursed, wildcardMatching);
}

static void AddToCensorFromListFile(
    NWildcard::CCensor& censor, LPCWSTR fileName, bool include, NRecursedType::EEnum type) {
  UStringVector names;
  if (!NFind::DoesFileExist(us2fs(fileName)))
    throw CArcCmdLineException(kCannotFindListFile, fileName);
  DWORD lastError = 0;
  if (!ReadNamesFromListFile2(us2fs(fileName), names, CP_UTF8, lastError)) {
    if (lastError != 0) {
      UString m;
      m = "The file operation error for listfile";
      m.Add_LF();
      m += NError::MyFormatMessage(lastError);
      throw CArcCmdLineException(m, fileName);
    }
    throw CArcCmdLineException(kIncorrectListFile, fileName);
  }

  FOR_VECTOR(i, names)
  AddNameToCensor(censor, names[i], include, type, false);
}

static void AddToCensorFromNonSwitchesStrings(
    unsigned startIndex, NWildcard::CCensor& censor, const UStringVector& nonSwitchStrings,
    int stopSwitchIndex) {
  if (nonSwitchStrings.Size() == startIndex)
    AddNameToCensor(
        censor, UString(kUniversalWildcard), true, NRecursedType::kNonRecursed,
        true  // wildcardMatching
    );

  int oldIndex = -1;

  if (stopSwitchIndex < 0) stopSwitchIndex = nonSwitchStrings.Size();

  for (unsigned i = startIndex; i < nonSwitchStrings.Size(); i++) {
    const UString& s = nonSwitchStrings[i];
    if (s.IsEmpty()) throw CArcCmdLineException(kEmptyFilePath);
    if (i < (unsigned)stopSwitchIndex && s[0] == kFileListID)
      AddToCensorFromListFile(censor, s.Ptr(1), true, NRecursedType::kNonRecursed);
    else
      AddNameToCensor(censor, s, true, NRecursedType::kNonRecursed, false);
  }

  if (oldIndex != -1) {
    throw CArcCmdLineException(
        "There is no second file name for rename pair:", nonSwitchStrings[oldIndex]);
  }
}

#ifdef _WIN32

struct CEventSetEnd {
  UString Name;

  CEventSetEnd(const wchar_t* name) : Name(name) {}
  ~CEventSetEnd() {
    NSynchronization::CManualResetEvent event;
    if (event.Open(EVENT_MODIFY_STATE, false, GetSystemString(Name)) == 0) event.Set();
  }
};

static const char* const k_IncorrectMapCommand = "Incorrect Map command";

#endif

static const int kUpdatePairStateNotSupportedActions[] = {2, 2, 1, -1, -1, -1, -1};

static const unsigned kNumUpdatePairActions = 4;
static const char* const kUpdateIgnoreItselfPostStringID = "-";
static const wchar_t kUpdateNewArchivePostCharID = '!';

bool ParseComplexSize(const wchar_t* s, UInt64& result);

static inline void SetStreamMode(const CSwitchResult& sw, unsigned& res) {
  if (sw.ThereIs) res = sw.PostCharIndex;
}

void CArcCmdLineParser::Parse1(const UStringVector& commandStrings, CArcCmdLineOptions& options) {
  if (!parser.ParseStrings(kSwitchForms, ARRAY_SIZE(kSwitchForms), commandStrings))
    throw CArcCmdLineException(parser.ErrorMessage, parser.ErrorLine);

  options.IsInTerminal = MY_IS_TERMINAL(stdin);
  options.IsStdOutTerminal = MY_IS_TERMINAL(stdout);
  options.IsStdErrTerminal = MY_IS_TERMINAL(stderr);

  if (!options.IsStdOutTerminal) options.Number_for_Percents = k_OutStream_disabled;

#if defined(_WIN32) && !defined(UNDER_CE)
  NSecurity::EnablePrivilege_SymLink();
#endif

  // options.LargePages = false;
}

void CArcCmdLineParser::Parse2(CArcCmdLineOptions& options) {
  const UStringVector& nonSwitchStrings = parser.NonSwitchStrings;
  const unsigned numNonSwitchStrings = nonSwitchStrings.Size();
  if (numNonSwitchStrings < kMinNonSwitchWords)
    throw CArcCmdLineException("The command must be specified");

  NWildcard::ECensorPathMode censorPathMode = NWildcard::k_RelatPath;

  g_CodePage = -1;

  unsigned curCommandIndex = kCommandIndex;

  if (curCommandIndex >= numNonSwitchStrings)
    throw CArcCmdLineException("Cannot find archive name");
  options.ArchiveName = nonSwitchStrings[curCommandIndex++];
  if (options.ArchiveName.IsEmpty()) throw CArcCmdLineException("Archive name cannot by empty");
#ifdef _WIN32
// options.ArchiveName.Replace(L'/', WCHAR_PATH_SEPARATOR);
#endif

  AddToCensorFromNonSwitchesStrings(
      curCommandIndex, options.Censor, nonSwitchStrings, parser.StopSwitchIndex);

#ifndef _NO_CRYPTO
  options.PasswordEnabled = parser[NKey::kPassword].ThereIs;
  if (options.PasswordEnabled) options.Password = parser[NKey::kPassword].PostStrings[0];
#endif

  CExtractOptionsBase& eo = options.ExtractOptions;

  {
    CExtractNtOptions& nt = eo.NtOptions;
    nt.NtSecurity = options.NtSecurity;

    nt.AltStreams = options.AltStreams;
    if (!options.AltStreams.Def) nt.AltStreams.Val = true;

    nt.HardLinks = options.HardLinks;
    if (!options.HardLinks.Def) nt.HardLinks.Val = true;

    nt.SymLinks = options.SymLinks;
    if (!options.SymLinks.Def) nt.SymLinks.Val = true;
  }

  options.Censor.AddPathsToCensor(NWildcard::k_AbsPath);
  options.Censor.ExtendExclude();

  // are there paths that look as non-relative (!Prefix.IsEmpty())
  if (!options.Censor.AllAreRelative())
    throw CArcCmdLineException("Cannot use absolute pathnames for this command");

  NWildcard::CCensor& arcCensor = options.arcCensor;

  AddNameToCensor(arcCensor, options.ArchiveName, true, NRecursedType::kNonRecursed, false);

  arcCensor.AddPathsToCensor(NWildcard::k_RelatPath);

#ifdef _WIN32
  ConvertToLongNames(arcCensor);
#endif

  arcCensor.ExtendExclude();

  if (parser[NKey::kOutputDir].ThereIs) {
    eo.OutputDir = us2fs(parser[NKey::kOutputDir].PostStrings[0]);
    NFile::NName::NormalizeDirPathPrefix(eo.OutputDir);
  }

  eo.PathMode = NExtract::NPathMode::kFullPaths;
  if (censorPathMode == NWildcard::k_AbsPath) {
    eo.PathMode = NExtract::NPathMode::kAbsPaths;
    eo.PathMode_Force = true;
  } else if (censorPathMode == NWildcard::k_FullPath) {
    eo.PathMode = NExtract::NPathMode::kFullPaths;
    eo.PathMode_Force = true;
  }
}
