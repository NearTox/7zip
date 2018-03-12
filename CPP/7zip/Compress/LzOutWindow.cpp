// LzOutWindow.cpp

#include "../../Common/Common.h"
#include "LzOutWindow.h"

void CLzOutWindow::Init(bool solid) throw() {
  if(!solid)
    COutBuffer::Init();
#ifdef _NO_EXCEPTIONS
  ErrorCode = S_OK;
#endif
}