// 7zAes.cpp

#include "../../Common/Common.h"

#include "../../../C/Sha256.h"

#include "../../Common/ComTry.h"

#ifndef _7ZIP_ST
#  include "../../Windows/Synchronization.h"
#endif

#include "../Common/StreamUtils.h"

#include "7zAes.h"
#include "MyAes.h"

namespace NCrypto {
namespace N7z {

static const unsigned k_NumCyclesPower_Supported_MAX = 24;

bool CKeyInfo::IsEqualTo(const CKeyInfo& a) const {
  if (SaltSize != a.SaltSize || NumCyclesPower != a.NumCyclesPower) return false;
  for (unsigned i = 0; i < SaltSize; i++)
    if (Salt[i] != a.Salt[i]) return false;
  return (Password == a.Password);
}

void CKeyInfo::CalcKey() {
  if (NumCyclesPower == 0x3F) {
    unsigned pos;
    for (pos = 0; pos < SaltSize; pos++) Key[pos] = Salt[pos];
    for (unsigned i = 0; i < Password.Size() && pos < kKeySize; i++) Key[pos++] = Password[i];
    for (; pos < kKeySize; pos++) Key[pos] = 0;
  } else {
    size_t bufSize = 8 + SaltSize + Password.Size();
    CObjArray<Byte> buf(bufSize);
    memcpy(buf, Salt, SaltSize);
    memcpy(buf + SaltSize, Password, Password.Size());

    CSha256 sha;
    Sha256_Init(&sha);

    Byte* ctr = buf + SaltSize + Password.Size();

    for (unsigned i = 0; i < 8; i++) ctr[i] = 0;

    UInt64 numRounds = (UInt64)1 << NumCyclesPower;

    do {
      Sha256_Update(&sha, buf, bufSize);
      for (unsigned i = 0; i < 8; i++)
        if (++(ctr[i]) != 0) break;
    } while (--numRounds != 0);

    Sha256_Final(&sha, Key);
  }
}

bool CKeyInfoCache::GetKey(CKeyInfo& key) {
  FOR_VECTOR(i, Keys) {
    const CKeyInfo& cached = Keys[i];
    if (key.IsEqualTo(cached)) {
      for (unsigned j = 0; j < kKeySize; j++) key.Key[j] = cached.Key[j];
      if (i != 0) Keys.MoveToFront(i);
      return true;
    }
  }
  return false;
}

void CKeyInfoCache::FindAndAdd(const CKeyInfo& key) {
  FOR_VECTOR(i, Keys) {
    const CKeyInfo& cached = Keys[i];
    if (key.IsEqualTo(cached)) {
      if (i != 0) Keys.MoveToFront(i);
      return;
    }
  }
  Add(key);
}

void CKeyInfoCache::Add(const CKeyInfo& key) {
  if (Keys.Size() >= Size) Keys.DeleteBack();
  Keys.Insert(0, key);
}

static CKeyInfoCache g_GlobalKeyCache(32);

#ifndef _7ZIP_ST
static NWindows::NSynchronization::CCriticalSection g_GlobalKeyCacheCriticalSection;
#  define MT_LOCK \
    NWindows::NSynchronization::CCriticalSectionLock lock(g_GlobalKeyCacheCriticalSection);
#else
#  define MT_LOCK
#endif

CBase::CBase() : _cachedKeys(16), _ivSize(0) {
  for (unsigned i = 0; i < sizeof(_iv); i++) _iv[i] = 0;
}

void CBase::PrepareKey() {
  // BCJ2 threads use same password. So we use long lock.
  MT_LOCK

  bool finded = false;
  if (!_cachedKeys.GetKey(_key)) {
    finded = g_GlobalKeyCache.GetKey(_key);
    if (!finded) _key.CalcKey();
    _cachedKeys.Add(_key);
  }
  if (!finded) g_GlobalKeyCache.FindAndAdd(_key);
}

CDecoder::CDecoder() { _aesFilter = new CAesCbcDecoder(kKeySize); }

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte* data, UInt32 size) {
  _key.ClearProps();

  _ivSize = 0;
  unsigned i;
  for (i = 0; i < sizeof(_iv); i++) _iv[i] = 0;

  if (size == 0) return S_OK;

  Byte b0 = data[0];

  _key.NumCyclesPower = b0 & 0x3F;
  if ((b0 & 0xC0) == 0) return size == 1 ? S_OK : E_INVALIDARG;

  if (size <= 1) return E_INVALIDARG;

  Byte b1 = data[1];

  unsigned saltSize = ((b0 >> 7) & 1) + (b1 >> 4);
  unsigned ivSize = ((b0 >> 6) & 1) + (b1 & 0x0F);

  if (size != 2 + saltSize + ivSize) return E_INVALIDARG;
  _key.SaltSize = saltSize;
  data += 2;
  for (i = 0; i < saltSize; i++) _key.Salt[i] = *data++;
  for (i = 0; i < ivSize; i++) _iv[i] = *data++;
  return (_key.NumCyclesPower <= k_NumCyclesPower_Supported_MAX || _key.NumCyclesPower == 0x3F)
             ? S_OK
             : E_NOTIMPL;
}

STDMETHODIMP CBaseCoder::CryptoSetPassword(const Byte* data, UInt32 size) {
  COM_TRY_BEGIN

  _key.Password.CopyFrom(data, (size_t)size);
  return S_OK;

  COM_TRY_END
}

STDMETHODIMP CBaseCoder::Init() {
  COM_TRY_BEGIN

  PrepareKey();
  CMyComPtr<ICryptoProperties> cp;
  RINOK(_aesFilter.QueryInterface(IID_ICryptoProperties, &cp));
  if (!cp) return E_FAIL;
  RINOK(cp->SetKey(_key.Key, kKeySize));
  RINOK(cp->SetInitVector(_iv, sizeof(_iv)));
  return _aesFilter->Init();

  COM_TRY_END
}

STDMETHODIMP_(UInt32) CBaseCoder::Filter(Byte* data, UInt32 size) {
  return _aesFilter->Filter(data, size);
}

}  // namespace N7z
}  // namespace NCrypto
