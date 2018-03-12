// Windows/Handle.h

#ifndef __WINDOWS_HANDLE_H
#define __WINDOWS_HANDLE_H

namespace NWindows {
  class CHandle {
  protected:
    HANDLE _handle;
  public:
    operator HANDLE() {
      return _handle;
    }
    CHandle() : _handle(nullptr) {}
    ~CHandle() {
      Close();
    }
    bool IsCreated() const {
      return (_handle != nullptr);
    }
    bool Close() {
      if(_handle == nullptr)
        return true;
      if(!::CloseHandle(_handle))
        return false;
      _handle = nullptr;
      return true;
    }
    void Attach(HANDLE handle) {
      _handle = handle;
    }
    HANDLE Detach() {
      HANDLE handle = _handle;
      _handle = nullptr;
      return handle;
    }
  };
}

#endif
