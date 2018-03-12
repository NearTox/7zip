// OpenCallbackConsole.h

#ifndef __OPEN_CALLBACK_CONSOLE_H
#define __OPEN_CALLBACK_CONSOLE_H

#include "../../../Common/StdOutStream.h"

#include "../Common/ArchiveOpenCallback.h"
#include "../../../Common/StdOutStream.h"

class COpenCallbackConsole : public IOpenCallbackUI {
protected:
  CStdOutStream *_so;
  CStdOutStream *_se;

  bool _totalFilesDefined;
  bool _totalBytesDefined;

public:

  bool MultiArcMode;
  COpenCallbackConsole() : _totalFilesDefined(false), _totalBytesDefined(false), MultiArcMode(false), PasswordIsDefined(false) {}

  void Init(CStdOutStream *outStream, CStdOutStream *errorStream) {
    _so = outStream;
    _se = errorStream;
  }

  INTERFACE_IOpenCallbackUI_Crypto(;)

    bool PasswordIsDefined;
  // bool PasswordWasAsked;
  UString Password;
};

#endif
