// Lzma2Register.cpp

#include "../../Common/Common.h"

#include "../Common/RegisterCodec.h"

#include "Lzma2Decoder.h"

namespace NCompress {
namespace NLzma2 {

REGISTER_CODEC_E(LZMA2, CDecoder(), CEncoder(), 0x21, "LZMA2")

}
}  // namespace NCompress
