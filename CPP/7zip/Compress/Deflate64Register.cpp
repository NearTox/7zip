// Deflate64Register.cpp

#include "../../Common/Common.h"

#include "../Common/RegisterCodec.h"

#include "DeflateDecoder.h"

namespace NCompress {
  namespace NDeflate {
    REGISTER_CODEC_CREATE(CreateDec, NDecoder::CCOMCoder64())
      REGISTER_CODEC_2(Deflate64, CreateDec, 0x40109, "Deflate64")
  }
}