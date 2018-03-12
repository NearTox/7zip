// MainAr.cpp

#include "../../../Common/Common.h"
#include "../../../Common/MyException.h"
#include "../../../Common/StdOutStream.h"
#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/NtCheck.h"
#include "../Common/ArchiveCommandLine.h"
#include "../Common/ExitCode.h"

CStdOutStream *g_StdStream = nullptr;
CStdOutStream *g_ErrStream = nullptr;

int _7zExtractorCMD_2(const wchar_t *args);

static const char *kException_CmdLine_Error_Message = "Command Line Error:";
static const char *kExceptionErrorMessage = "ERROR:";
static const char *kUserBreakMessage = "Break signaled";
static const char *kMemoryExceptionMessage = "ERROR: Can't allocate required memory!";
static const char *kUnknownExceptionMessage = "Unknown Error";
static const char *kInternalExceptionMessage = "\n\nInternal Error #";

static void FlushStreams() {
  if(g_StdStream)
    g_StdStream->Flush();
}

static void PrintError(const char *message) {
  FlushStreams();
  if(g_ErrStream)
    *g_ErrStream << "\n\n" << message << endl;
}

#define NT_CHECK_FAIL_ACTION *g_StdStream << "Unsupported Windows version"; return NExitCode::kFatalError;

#ifdef useMain
int _7zExtractorCMD(const wchar_t *args);
int MY_CDECL main(
#ifndef _WIN32
  int numArgs, char *args[]
#endif
) {
  return _7zExtractorCMD(GetCommandLineW());
}
#endif
int _7zExtractorCMD(const wchar_t *args) {
  g_ErrStream = &g_StdErr;
  g_StdStream = &g_StdOut;

  NT_CHECK
    int res = 0;

  try {
    res = _7zExtractorCMD_2(args);
  } catch(const CNewException &) {
    PrintError(kMemoryExceptionMessage);
    return (NExitCode::kMemoryError);
  } catch(const CArcCmdLineException &e) {
    PrintError(kException_CmdLine_Error_Message);
    if(g_ErrStream)
      *g_ErrStream << e << endl;
    return (NExitCode::kUserError);
  } catch(const CSystemException &systemError) {
    if(systemError.ErrorCode == E_OUTOFMEMORY) {
      PrintError(kMemoryExceptionMessage);
      return (NExitCode::kMemoryError);
    }
    if(systemError.ErrorCode == E_ABORT) {
      PrintError(kUserBreakMessage);
      return (NExitCode::kUserBreak);
    }
    if(g_ErrStream) {
      PrintError("System ERROR:");
      *g_ErrStream << NWindows::NError::MyFormatMessage(systemError.ErrorCode) << endl;
    }
    return (NExitCode::kFatalError);
  } catch(NExitCode::EEnum &exitCode) {
    FlushStreams();
    if(g_ErrStream)
      *g_ErrStream << kInternalExceptionMessage << exitCode << endl;
    return (exitCode);
  } catch(const UString &s) {
    if(g_ErrStream) {
      PrintError(kExceptionErrorMessage);
      *g_ErrStream << s << endl;
    }
    return (NExitCode::kFatalError);
  } catch(const AString &s) {
    if(g_ErrStream) {
      PrintError(kExceptionErrorMessage);
      *g_ErrStream << s << endl;
    }
    return (NExitCode::kFatalError);
  } catch(const char *s) {
    if(g_ErrStream) {
      PrintError(kExceptionErrorMessage);
      *g_ErrStream << s << endl;
    }
    return (NExitCode::kFatalError);
  } catch(const wchar_t *s) {
    if(g_ErrStream) {
      PrintError(kExceptionErrorMessage);
      *g_ErrStream << s << endl;
    }
    return (NExitCode::kFatalError);
  } catch(int t) {
    if(g_ErrStream) {
      FlushStreams();
      *g_ErrStream << kInternalExceptionMessage << t << endl;
      return (NExitCode::kFatalError);
    }
  } catch(...) {
    PrintError(kUnknownExceptionMessage);
    return (NExitCode::kFatalError);
  }

  return res;
}