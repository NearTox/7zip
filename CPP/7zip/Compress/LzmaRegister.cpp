// LzmaRegister.cpp

#include "../../Common/Common.h"

#include "../Common/RegisterCodec.h"

#include "LzmaDecoder.h"

namespace NCompress {
namespace NLzma {

REGISTER_CODEC_E(LZMA, CDecoder(), CEncoder(), 0x30101, "LZMA")

}
}  // namespace NCompress
