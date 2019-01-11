// 7z/Handler.h

#ifndef __7Z_HANDLER_H
#define __7Z_HANDLER_H

#include "../../ICoder.h"
#include "../IArchive.h"

#include "../../Common/CreateCoder.h"

#ifndef __7Z_SET_PROPERTIES

#  if !defined(_7ZIP_ST) && !defined(_SFX)
#    define __7Z_SET_PROPERTIES
#  endif

#endif

// #ifdef __7Z_SET_PROPERTIES
#include "../Common/HandlerOut.h"
// #endif

#include "7zCompressionMode.h"
#include "7zIn.h"

namespace NArchive {
namespace N7z {

class CHandler
    : public IInArchive
    , public IArchiveGetRawProps
    ,
#ifdef __7Z_SET_PROPERTIES
      public ISetProperties
    ,
#endif



      public CMyUnknownImp
    ,

      public CCommonMethodProps {
 public:
  MY_QUERYINTERFACE_BEGIN2(IInArchive)
  MY_QUERYINTERFACE_ENTRY(IArchiveGetRawProps)
#ifdef __7Z_SET_PROPERTIES
  MY_QUERYINTERFACE_ENTRY(ISetProperties)
#endif
   MY_QUERYINTERFACE_END MY_ADDREF_RELEASE

      INTERFACE_IInArchive(;) INTERFACE_IArchiveGetRawProps(;)

#ifdef __7Z_SET_PROPERTIES
          STDMETHOD(SetProperties)(
              const wchar_t* const* names, const PROPVARIANT* values, UInt32 numProps);
#endif



  CHandler();

 private:
  CMyComPtr<IInStream> _inStream;
  NArchive::N7z::CDbEx _db;

#ifndef _NO_CRYPTO
  bool _isEncrypted;
  bool _passwordIsDefined;
  UString _password;
#endif

#ifdef __7Z_SET_PROPERTIES
  bool _useMultiThreadMixer;
#endif

  UInt32 _crcSize;

  bool IsFolderEncrypted(CNum folderIndex) const;
#ifndef _SFX

  CRecordVector<UInt64> _fileInfoPopIDs;
  void FillPopIDs();
  void AddMethodName(AString& s, UInt64 id);
  HRESULT SetMethodToProp(CNum folderIndex, PROPVARIANT* prop) const;

#endif


};

}  // namespace N7z
}  // namespace NArchive

#endif
