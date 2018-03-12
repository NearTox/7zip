// ArchiveCommandLine.h

#ifndef __ARCHIVE_COMMAND_LINE_H
#define __ARCHIVE_COMMAND_LINE_H

#include "../../../Common/CommandLineParser.h"
#include "../../../Common/Wildcard.h"
#include "Extract.h"

struct CArcCmdLineException : public UString {
  CArcCmdLineException(const char *a, const wchar_t *u = nullptr);
};

namespace NCommandType {
  enum EEnum {
    kTest = 0,
    kExtractFull
  };
}

struct CArcCmdLineOptions {
  bool IsInTerminal;
  bool IsStdOutTerminal;
  bool IsStdErrTerminal;

  NWildcard::CCensor Censor;

  NCommandType::EEnum CommandType;
  bool IsTestCommand() const {
    return CommandType == NCommandType::kTest;
  }
  UString ArchiveName;

  bool PasswordEnabled;
  UString Password;

  NWildcard::CCensor arcCensor;

  CExtractOptions ExtractOptions;
  CArcCmdLineOptions() {};
};

class CArcCmdLineParser {
  NCommandLineParser::CParser parser;
public:
  CArcCmdLineParser();
  void Parse1(const UStringVector &commandStrings, CArcCmdLineOptions &options);
  void Parse2(CArcCmdLineOptions &options);
};

HRESULT EnumerateDirItemsAndSort(NWildcard::CCensor &censor, UStringVector &sortedPaths, UStringVector &sortedFullPaths, CDirItemsStat &st, IDirItemsCallback *callback);

#endif
