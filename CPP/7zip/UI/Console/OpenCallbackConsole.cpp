// OpenCallbackConsole.cpp

#include "../../../Common/Common.h"
#include "OpenCallbackConsole.h"
#include "../../../Common/StdOutStream.h"
HRESULT COpenCallbackConsole::Open_CryptoGetTextPassword(BSTR *password) {
  *password = nullptr;
  if(!PasswordIsDefined) {
    Password = L"";
    PasswordIsDefined = true;
  }
  return StringToBstr(Password, password);
}