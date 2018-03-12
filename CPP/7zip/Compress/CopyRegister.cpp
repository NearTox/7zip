// CopyRegister.cpp

#include "../../Common/Common.h"

#include "../Common/RegisterCodec.h"

#include "CopyCoder.h"

namespace NCompress {
  REGISTER_CODEC_CREATE(CreateCodec, CCopyCoder())
    REGISTER_CODEC_2(Copy, CreateCodec, 0, "Copy")
}