// ArchiveCommandLine.h

#ifndef __ARCHIVE_COMMAND_LINE_H
#define __ARCHIVE_COMMAND_LINE_H

#include "../../../Common/CommandLineParser.h"
#include "../../../Common/Wildcard.h"

#include "EnumDirItems.h"

#include "Extract.h"
#include "HashCalc.h"
#include "Update.h"

typedef CMessagePathException CArcCmdLineException;

enum { k_OutStream_disabled = 0, k_OutStream_stdout = 1, k_OutStream_stderr = 2 };

struct CArcCmdLineOptions {
  // bool LargePages;
  bool CaseSensitiveChange;
  bool CaseSensitive;

  bool IsInTerminal;
  bool IsStdOutTerminal;
  bool IsStdErrTerminal;

  NWildcard::CCensor Censor;

  UString ArchiveName;

#ifndef _NO_CRYPTO
  bool PasswordEnabled;
  UString Password;
#endif

  UStringVector HashMethods;

  bool AppendName;
  // UStringVector ArchivePathsSorted;
  // UStringVector ArchivePathsFullSorted;
  NWildcard::CCensor arcCensor;
  UString ArcName_for_StdInMode;

  CObjectVector<CProperty> Properties;

  CExtractOptionsBase ExtractOptions;

  CBoolPair NtSecurity;
  CBoolPair AltStreams;
  CBoolPair HardLinks;
  CBoolPair SymLinks;

  UString ArcType;
  UStringVector ExcludedArcTypes;

  unsigned Number_for_Out;
  unsigned Number_for_Errors;
  unsigned Number_for_Percents;
  unsigned LogLevel;

  // bool IsOutAllowed() const { return Number_for_Out != k_OutStream_disabled; }

  // Benchmark
  UInt32 NumIterations;

  CArcCmdLineOptions() :
      // LargePages(false),
      CaseSensitiveChange(false),
      CaseSensitive(false),

      Number_for_Out(k_OutStream_stdout),
      Number_for_Errors(k_OutStream_stderr),
      Number_for_Percents(k_OutStream_stdout),

      LogLevel(0){};
};

class CArcCmdLineParser {
  NCommandLineParser::CParser parser;

 public:
  void Parse1(const UStringVector& commandStrings, CArcCmdLineOptions& options);
  void Parse2(CArcCmdLineOptions& options);
};

#endif
