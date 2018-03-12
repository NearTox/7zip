// 7zAesRegister.cpp

#include "../../Common/Common.h"

#include "../Common/RegisterCodec.h"

#include "7zAes.h"

REGISTER_FILTER_E(7zAES, NCrypto::N7z::CDecoder(), 0x6F10701, "7zAES")