// ArchiveCommandLine.cpp

#include "../../../Common/Common.h"
#undef printf
#undef sprintf

#ifdef _WIN32
#ifndef UNDER_CE
#include <io.h>
#endif
#endif
#include <stdio.h>
#include "../../../Common/StringConvert.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileName.h"
#include "ArchiveCommandLine.h"
#include "EnumDirItems.h"
#include "SortUtils.h"

extern bool g_CaseSensitive;

#ifdef UNDER_CE

#define MY_IS_TERMINAL(x) false;

#else

#if _MSC_VER >= 1400
#define MY_isatty_fileno(x) _isatty(_fileno(x))
#else
#define MY_isatty_fileno(x) isatty(fileno(x))
#endif

#define MY_IS_TERMINAL(x) (MY_isatty_fileno(x) != 0);

#endif

CArcCmdLineException::CArcCmdLineException(const char *a, const wchar_t *u) {
  (*this) += MultiByteToUnicodeString(a);
  if(u) {
    this->Add_LF();
    (*this) += u;
  }
}
namespace NKey {
  enum Enum {
    kOutputDir = 0
    , kPassword
  };
}
namespace NRecursedPostCharIndex {
  enum EEnum {
    kWildcardRecursionOnly = 0,
    kNoRecursion = 1
  };
}

static const NCommandLineParser::CSwitchForm kSwitchForms[] = {
  {"o", NCommandLineParser::NSwitchType::kString, false, 1}
  , {"p", NCommandLineParser::NSwitchType::kString}
};

static const wchar_t *kUniversalWildcard = L"*";
static const unsigned kMinNonSwitchWords = 1;
static const unsigned kCommandIndex = 0;

// static const char *kUserErrorMessage  = "Incorrect command line";
static const char *kCannotFindListFile = "Cannot find listfile";
static const char *kIncorrectListFile = "Incorrect item in listfile.\nCheck charset encoding and -scs switch.";
//static const char *kTerminalOutError = "I won't write compressed data to a terminal";
//static const char *kSameTerminalError = "I won't write data and program's messages to same stream";
static const char *kEmptyFilePath = "Empty file path";
static const char *kCannotFindArchive = "Cannot find archive";

static const char *g_Commands = "tx";
static bool ParseArchiveCommand(const UString &commandString, NCommandType::EEnum &command) {
  UString s = commandString;
  s.MakeLower_Ascii();
  if(s.Len() == 1) {
    if(s[0] > 0x7F)
      return false;
    int index = FindCharPosInString(g_Commands, (char)s[0]);
    if(index < 0)
      return false;
    command = (NCommandType::EEnum)index;
    return true;
  }
  return false;
}

// ------------------------------------------------------------------
// filenames functions

#ifdef _WIN32

// This code converts all short file names to long file names.

static void ConvertToLongName(const UString &prefix, UString &name) {
  if(name.IsEmpty() || DoesNameContainWildcard(name))
    return;
  NWindows::NFile::NFind::CFileInfo fi;
  const FString path = us2fs(prefix + name);
#ifndef UNDER_CE
  if(NWindows::NFile::NName::IsDevicePath(path))
    return;
#endif
  if(fi.Find(path))
    name = fs2us(fi.Name);
}

static void ConvertToLongNames(const UString &prefix, CObjectVector<NWildcard::CItem> &items) {
  FOR_VECTOR(i, items) {
    NWildcard::CItem &item = items[i];
    if(item.Recursive || item.PathParts.Size() != 1)
      continue;
    if(prefix.IsEmpty() && item.IsDriveItem())
      continue;
    ConvertToLongName(prefix, item.PathParts.Front());
  }
}

static void ConvertToLongNames(const UString &prefix, NWildcard::CCensorNode &node) {
  ConvertToLongNames(prefix, node.IncludeItems);
  ConvertToLongNames(prefix, node.ExcludeItems);
  unsigned i;
  for(i = 0; i < node.SubNodes.Size(); i++) {
    UString &name = node.SubNodes[i].Name;
    if(prefix.IsEmpty() && NWildcard::IsDriveColonName(name))
      continue;
    ConvertToLongName(prefix, name);
  }
  // mix folders with same name
  for(i = 0; i < node.SubNodes.Size(); i++) {
    NWildcard::CCensorNode &nextNode1 = node.SubNodes[i];
    for(unsigned j = i + 1; j < node.SubNodes.Size();) {
      const NWildcard::CCensorNode &nextNode2 = node.SubNodes[j];
      if(nextNode1.Name.IsEqualTo_NoCase(nextNode2.Name)) {
        nextNode1.IncludeItems += nextNode2.IncludeItems;
        nextNode1.ExcludeItems += nextNode2.ExcludeItems;
        node.SubNodes.Delete(j);
      } else
        j++;
    }
  }
  for(i = 0; i < node.SubNodes.Size(); i++) {
    NWildcard::CCensorNode &nextNode = node.SubNodes[i];
    ConvertToLongNames(prefix + nextNode.Name + WCHAR_PATH_SEPARATOR, nextNode);
  }
}

void ConvertToLongNames(NWildcard::CCensor &censor) {
  FOR_VECTOR(i, censor.Pairs) {
    NWildcard::CPair &pair = censor.Pairs[i];
    ConvertToLongNames(pair.Prefix, pair.Head);
  }
}

#endif

//static const unsigned kNumUpdatePairActions = 4;
//static const char *kUpdateIgnoreItselfPostStringID = "-";
//static const wchar_t kUpdateNewArchivePostCharID = '!';

bool ParseComplexSize(const wchar_t *s, UInt64 &result);
CArcCmdLineParser::CArcCmdLineParser() : parser(ARRAY_SIZE(kSwitchForms)) {}

void CArcCmdLineParser::Parse1(const UStringVector &commandStrings, CArcCmdLineOptions &options) {
  if(!parser.ParseStrings(kSwitchForms, commandStrings))
    throw CArcCmdLineException(parser.ErrorMessage, parser.ErrorLine);

  options.IsInTerminal = MY_IS_TERMINAL(stdin);
  options.IsStdOutTerminal = MY_IS_TERMINAL(stdout);
  options.IsStdErrTerminal = MY_IS_TERMINAL(stderr);
}

//static const unsigned kNumByteOnlyCodePages = 3;

HRESULT EnumerateDirItemsAndSort(NWildcard::CCensor &censor, UStringVector &sortedPaths, UStringVector &sortedFullPaths, CDirItemsStat &st, IDirItemsCallback *callback) {
  FStringVector paths;

  {
    CDirItems dirItems;
    dirItems.Callback = callback;
    HRESULT res = EnumerateItems(censor, dirItems);
    st = dirItems.Stat;
    RINOK(res);

    FOR_VECTOR(i, dirItems.Items) {
      const CDirItem &dirItem = dirItems.Items[i];
      if(!dirItem.IsDir())
        paths.Add(dirItems.GetPhyPath(i));
    }
  }

  if(paths.Size() == 0)
    throw CArcCmdLineException(kCannotFindArchive);

  UStringVector fullPaths;

  unsigned i;

  for(i = 0; i < paths.Size(); i++) {
    FString fullPath;
    NWindows::NFile::NDir::MyGetFullPathName(paths[i], fullPath);
    fullPaths.Add(fs2us(fullPath));
  }

  CUIntVector indices;
  SortFileNames(fullPaths, indices);
  sortedPaths.ClearAndReserve(indices.Size());
  sortedFullPaths.ClearAndReserve(indices.Size());

  for(i = 0; i < indices.Size(); i++) {
    unsigned index = indices[i];
    sortedPaths.AddInReserved(fs2us(paths[index]));
    sortedFullPaths.AddInReserved(fullPaths[index]);
    if(i > 0 && CompareFileNames(sortedFullPaths[i], sortedFullPaths[i - 1]) == 0)
      throw CArcCmdLineException("Duplicate archive path:", sortedFullPaths[i]);
  }

  return S_OK;
}

void CArcCmdLineParser::Parse2(CArcCmdLineOptions &options) {
  unsigned numNonSwitchStrings = parser.NonSwitchStrings.Size();
  if(numNonSwitchStrings < kMinNonSwitchWords)
    throw CArcCmdLineException("The command must be spcified");

  if(!ParseArchiveCommand(parser.NonSwitchStrings[kCommandIndex], options.CommandType))
    throw CArcCmdLineException("Unsupported command:", parser.NonSwitchStrings[kCommandIndex]);

  unsigned curCommandIndex = kCommandIndex + 1;
  if(curCommandIndex >= numNonSwitchStrings)
    throw CArcCmdLineException("Cannot find archive name");
  options.ArchiveName = parser.NonSwitchStrings[curCommandIndex++];
  if(options.ArchiveName.IsEmpty())
    throw CArcCmdLineException("Archive name cannot by empty");

  if((parser.NonSwitchStrings.Size() == curCommandIndex))
    options.Censor.AddPreItem(true, kUniversalWildcard, false, true);

  options.PasswordEnabled = parser[NKey::kPassword].ThereIs;
  if(options.PasswordEnabled)
    options.Password = parser[NKey::kPassword].PostStrings[0];

  options.Censor.AddPathsToCensor(NWildcard::k_AbsPath);
  options.Censor.ExtendExclude();

  // are there paths that look as non-relative (!Prefix.IsEmpty())
  if(!options.Censor.AllAreRelative())
    throw CArcCmdLineException("Cannot use absolute pathnames for this command");

  options.arcCensor.AddPreItem(true, options.ArchiveName, false, true);

  options.arcCensor.AddPathsToCensor(NWildcard::k_RelatPath);

#ifdef _WIN32
  ConvertToLongNames(options.arcCensor);
#endif

  options.arcCensor.ExtendExclude();
  if(parser[NKey::kOutputDir].ThereIs) {
    options.ExtractOptions.OutputDir = us2fs(parser[NKey::kOutputDir].PostStrings[0]);
    NWindows::NFile::NName::NormalizeDirPathPrefix(options.ExtractOptions.OutputDir);
  }
}