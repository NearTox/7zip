// Crypto/MyAes.h

#ifndef __CRYPTO_MY_AES_H
#define __CRYPTO_MY_AES_H

#include "../../../C/Aes.h"

#include "../../Common/MyCom.h"

#include "../ICoder.h"

namespace NCrypto {
  class CAesCbcCoder : public ICompressFilter, public ICryptoProperties, public ICompressSetCoderProperties, public CMyUnknownImp {
    AES_CODE_FUNC _codeFunc;
    unsigned _offset;
    unsigned _keySize;
    bool _keyIsSet;
    UInt32 _aes[AES_NUM_IVMRK_WORDS + 3];
    Byte _iv[AES_BLOCK_SIZE];

    bool SetFunctions(UInt32 algo);

  public:
    CAesCbcCoder(unsigned keySize);

    MY_UNKNOWN_IMP2(ICryptoProperties, ICompressSetCoderProperties)

      INTERFACE_ICompressFilter(;)

      STDMETHOD(SetKey)(const Byte *data, UInt32 size);
    STDMETHOD(SetInitVector)(const Byte *data, UInt32 size);

    STDMETHOD(SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
  };

  struct CAesCbcDecoder : public CAesCbcCoder {
    CAesCbcDecoder(unsigned keySize = 0) : CAesCbcCoder(keySize) {}
  };
}

#endif
