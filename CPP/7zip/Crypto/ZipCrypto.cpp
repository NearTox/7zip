// Crypto/ZipCrypto.cpp

#include "../../Common/Common.h"

#include "../../../C/7zCrc.h"

#include "../Common/StreamUtils.h"

#include "RandGen.h"
#include "ZipCrypto.h"

namespace NCrypto {
  namespace NZip {
#define UPDATE_KEYS(b) { \
  Key0 = CRC_UPDATE_BYTE(Key0, b); \
  Key1 = (Key1 + (Key0 & 0xFF)) * 0x8088405 + 1; \
  Key2 = CRC_UPDATE_BYTE(Key2, (Byte)(Key1 >> 24)); } \

#define DECRYPT_BYTE_1 UInt32 temp = Key2 | 2;
#define DECRYPT_BYTE_2 ((Byte)((temp * (temp ^ 1)) >> 8))

    STDMETHODIMP CCipher::CryptoSetPassword(const Byte *data, UInt32 size) {
      UInt32 Key0 = 0x12345678;
      UInt32 Key1 = 0x23456789;
      UInt32 Key2 = 0x34567890;

      for(UInt32 i = 0; i < size; i++)
        UPDATE_KEYS(data[i]);

      KeyMem0 = Key0;
      KeyMem1 = Key1;
      KeyMem2 = Key2;

      return S_OK;
    }

    STDMETHODIMP CCipher::Init() {
      return S_OK;
    }

    HRESULT CDecoder::ReadHeader(ISequentialInStream *inStream) {
      return ReadStream_FAIL(inStream, _header, kHeaderSize);
    }

    void CDecoder::Init_BeforeDecode() {
      RestoreKeys();
      Filter(_header, kHeaderSize);
    }

    STDMETHODIMP_(UInt32) CDecoder::Filter(Byte *data, UInt32 size) {
      UInt32 Key0 = this->Key0;
      UInt32 Key1 = this->Key1;
      UInt32 Key2 = this->Key2;

      for(UInt32 i = 0; i < size; i++) {
        DECRYPT_BYTE_1
          Byte b = (Byte)(data[i] ^ DECRYPT_BYTE_2);
        UPDATE_KEYS(b);
        data[i] = b;
      }

      this->Key0 = Key0;
      this->Key1 = Key1;
      this->Key2 = Key2;

      return size;
    }
  }
}