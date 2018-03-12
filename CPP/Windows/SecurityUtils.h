// Windows/SecurityUtils.h

#ifndef __WINDOWS_SECURITY_UTILS_H
#define __WINDOWS_SECURITY_UTILS_H

#include <NTSecAPI.h>

#include "Defs.h"

namespace NWindows {
  namespace NSecurity {
    class CAccessToken {
      HANDLE _handle;
    public:
      CAccessToken() : _handle(nullptr) {};
      ~CAccessToken() {
        Close();
      }
      bool Close() {
        if(_handle == nullptr)
          return true;
        bool res = BOOLToBool(::CloseHandle(_handle));
        if(res)
          _handle = nullptr;
        return res;
      }

      bool OpenProcessToken(HANDLE processHandle, DWORD desiredAccess) {
        Close();
        return BOOLToBool(::OpenProcessToken(processHandle, desiredAccess, &_handle));
      }

      /*
      bool OpenThreadToken(HANDLE threadHandle, DWORD desiredAccess, bool openAsSelf)
      {
        Close();
        return BOOLToBool(::OpenTreadToken(threadHandle, desiredAccess, BoolToBOOL(anOpenAsSelf), &_handle));
      }
      */

      bool AdjustPrivileges(bool disableAllPrivileges, PTOKEN_PRIVILEGES newState, DWORD bufferLength, PTOKEN_PRIVILEGES previousState, PDWORD returnLength) {
        return BOOLToBool(::AdjustTokenPrivileges(_handle, BoolToBOOL(disableAllPrivileges),
                                                  newState, bufferLength, previousState, returnLength));
      }

      bool AdjustPrivileges(bool disableAllPrivileges, PTOKEN_PRIVILEGES newState) {
        return AdjustPrivileges(disableAllPrivileges, newState, 0, nullptr, nullptr);
      }

      bool AdjustPrivileges(PTOKEN_PRIVILEGES newState) {
        return AdjustPrivileges(false, newState);
      }
    };

#ifndef _UNICODE
    typedef NTSTATUS(NTAPI *LsaOpenPolicyP)(PLSA_UNICODE_STRING SystemName, PLSA_OBJECT_ATTRIBUTES ObjectAttributes, ACCESS_MASK DesiredAccess, PLSA_HANDLE PolicyHandle);
    typedef NTSTATUS(NTAPI *LsaCloseP)(LSA_HANDLE ObjectHandle);
    typedef NTSTATUS(NTAPI *LsaAddAccountRightsP)(LSA_HANDLE PolicyHandle, PSID AccountSid, PLSA_UNICODE_STRING UserRights, ULONG CountOfRights);
#define MY_STATUS_NOT_IMPLEMENTED           ((NTSTATUS)0xC0000002L)
#endif
  }
}

#endif
