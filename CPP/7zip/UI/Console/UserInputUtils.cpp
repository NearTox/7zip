// UserInputUtils.cpp

#include "../../../Common/Common.h"

#include "../../../Common/StdInStream.h"
#include "../../../Common/StringConvert.h"

#include "UserInputUtils.h"

#ifdef _WIN32
#  ifndef UNDER_CE
#    define MY_DISABLE_ECHO
#  endif
#endif

static bool GetPassword(CStdOutStream* outStream, UString& psw) {
  if (outStream) {
    *outStream << "\nEnter password"
#ifdef MY_DISABLE_ECHO
                  " (will not be echoed)"
#endif
                  ":";
    outStream->Flush();
  }

#ifdef MY_DISABLE_ECHO

  HANDLE console = GetStdHandle(STD_INPUT_HANDLE);
  bool wasChanged = false;
  DWORD mode = 0;
  if (console != INVALID_HANDLE_VALUE && console != 0)
    if (GetConsoleMode(console, &mode))
      wasChanged = (SetConsoleMode(console, mode & ~ENABLE_ECHO_INPUT) != 0);
  bool res = g_StdIn.ScanUStringUntilNewLine(psw);
  if (wasChanged) SetConsoleMode(console, mode);

#else

  bool res = g_StdIn.ScanUStringUntilNewLine(psw);

#endif

  if (outStream) {
    *outStream << endl;
    outStream->Flush();
  }

  return res;
}

HRESULT GetPassword_HRESULT(CStdOutStream* outStream, UString& psw) {
  if (!GetPassword(outStream, psw)) return E_INVALIDARG;
  if (g_StdIn.Error()) return E_FAIL;
  if (g_StdIn.Eof() && psw.IsEmpty()) return E_ABORT;
  return S_OK;
}
