// DeflateRegister.cpp

#include "../../Common/Common.h"
#include "../Common/RegisterCodec.h"

#include "DeflateDecoder.h"
#if !defined(EXTRACT_ONLY) && !defined(DEFLATE_EXTRACT_ONLY)
#include "DeflateEncoder.h"
#endif

namespace NCompress {
  namespace NDeflate {
    REGISTER_CODEC_CREATE(CreateDec, NDecoder::CCOMCoder)
      REGISTER_CODEC_2(Deflate, CreateDec, 0x40108, "Deflate")
  }
}