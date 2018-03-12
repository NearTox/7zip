/* Bcj2.h -- BCJ2 Converter for x86 code
2014-11-10 : Igor Pavlov : Public domain */

#ifndef __BCJ2_H
#define __BCJ2_H

#include "7zTypes.h"

EXTERN_C_BEGIN

#define BCJ2_NUM_STREAMS 4

enum {
  BCJ2_STREAM_MAIN,
  BCJ2_STREAM_CALL,
  BCJ2_STREAM_JUMP,
  BCJ2_STREAM_RC
};

enum {
  BCJ2_DEC_STATE_ORIG_0 = BCJ2_NUM_STREAMS,
  BCJ2_DEC_STATE_ORIG_1,
  BCJ2_DEC_STATE_ORIG_2,
  BCJ2_DEC_STATE_ORIG_3,

  BCJ2_DEC_STATE_ORIG,
  BCJ2_DEC_STATE_OK
};

enum {
  BCJ2_ENC_STATE_ORIG = BCJ2_NUM_STREAMS,
  BCJ2_ENC_STATE_OK
};

#define BCJ2_IS_32BIT_STREAM(s) ((s) == BCJ2_STREAM_CALL || (s) == BCJ2_STREAM_JUMP)

/*
CBcj2Dec / CBcj2Enc
bufs sizes:
  BUF_SIZE(n) = lims[n] - bufs[n]
bufs sizes for BCJ2_STREAM_CALL and BCJ2_STREAM_JUMP must be mutliply of 4:
    (BUF_SIZE(BCJ2_STREAM_CALL) & 3) == 0
    (BUF_SIZE(BCJ2_STREAM_JUMP) & 3) == 0
*/

/*
CBcj2Dec:
dest is allowed to overlap with bufs[BCJ2_STREAM_MAIN], with the following conditions:
  bufs[BCJ2_STREAM_MAIN] >= dest &&
  bufs[BCJ2_STREAM_MAIN] - dest >= tempReserv +
        BUF_SIZE(BCJ2_STREAM_CALL) +
        BUF_SIZE(BCJ2_STREAM_JUMP)
     tempReserv = 0 : for first call of Bcj2Dec_Decode
     tempReserv = 4 : for any other calls of Bcj2Dec_Decode
  overlap with offset = 1 is not allowed
*/

typedef struct {
  const Byte *bufs[BCJ2_NUM_STREAMS];
  const Byte *lims[BCJ2_NUM_STREAMS];
  Byte *dest;
  const Byte *destLim;

  unsigned state; /* BCJ2_STREAM_MAIN has more priority than BCJ2_STATE_ORIG */

  UInt32 ip;
  Byte temp[4];
  UInt32 range;
  UInt32 code;
  UInt16 probs[2 + 256];
} CBcj2Dec;

void Bcj2Dec_Init(CBcj2Dec *p);

/* Returns: SZ_OK or SZ_ERROR_DATA */
SRes Bcj2Dec_Decode(CBcj2Dec *p);

#define Bcj2Dec_IsFinished(_p_) ((_p_)->code == 0)

#define Bcj2Enc_Get_InputData_Size(p) ((SizeT)((p)->srcLim - (p)->src) + (p)->tempPos)
#define Bcj2Enc_IsFinished(p) ((p)->flushPos == 5)

#define BCJ2_RELAT_LIMIT_NUM_BITS 26
#define BCJ2_RELAT_LIMIT ((UInt32)1 << BCJ2_RELAT_LIMIT_NUM_BITS)

/* limit for CBcj2Enc::fileSize variable */
#define BCJ2_FileSize_MAX ((UInt32)1 << 31)

EXTERN_C_END

#endif
