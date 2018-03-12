// BZip2Register.cpp

#include "../../Common/Common.h"

#include "../Common/RegisterCodec.h"

#include "BZip2Decoder.h"

namespace NCompress {
  namespace NBZip2 {
    REGISTER_CODEC_CREATE(CreateDec, CDecoder)
      REGISTER_CODEC_2(BZip2, CreateDec, 0x40202, "BZip2")
  }
}