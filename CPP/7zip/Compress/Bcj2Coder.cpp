// Bcj2Coder.cpp

#include "../../Common/Common.h"
#include "../../../C/Alloc.h"

#include "../Common/StreamUtils.h"

#include "Bcj2Coder.h"

namespace NCompress {
  namespace NBcj2 {
    CBaseCoder::CBaseCoder() {
      for(int i = 0; i < BCJ2_NUM_STREAMS + 1; i++) {
        _bufs[i] = nullptr;
        _bufsCurSizes[i] = 0;
        _bufsNewSizes[i] = (1 << 18);
      }
    }

    CBaseCoder::~CBaseCoder() {
      for(int i = 0; i < BCJ2_NUM_STREAMS + 1; i++)
        ::MidFree(_bufs[i]);
    }

    HRESULT CBaseCoder::Alloc(bool allocForOrig) {
      unsigned num = allocForOrig ? BCJ2_NUM_STREAMS + 1 : BCJ2_NUM_STREAMS;
      for(unsigned i = 0; i < num; i++) {
        UInt32 newSize = _bufsNewSizes[i];
        const UInt32 kMinBufSize = 1;
        if(newSize < kMinBufSize)
          newSize = kMinBufSize;
        if(!_bufs[i] || newSize != _bufsCurSizes[i]) {
          if(_bufs[i]) {
            ::MidFree(_bufs[i]);
            _bufs[i] = 0;
          }
          _bufsCurSizes[i] = 0;
          Byte *buf = (Byte *)::MidAlloc(newSize);
          _bufs[i] = buf;
          if(!buf)
            return E_OUTOFMEMORY;
          _bufsCurSizes[i] = newSize;
        }
      }
      return S_OK;
    }

    STDMETHODIMP CDecoder::SetInBufSize(UInt32 streamIndex, UInt32 size) {
      _bufsNewSizes[streamIndex] = size; return S_OK;
    }
    STDMETHODIMP CDecoder::SetOutBufSize(UInt32, UInt32 size) {
      _bufsNewSizes[BCJ2_NUM_STREAMS] = size; return S_OK;
    }

    CDecoder::CDecoder() : _finishMode(false), _outSizeDefined(false), _outSize(0) {}

    STDMETHODIMP CDecoder::SetFinishMode(UInt32 finishMode) {
      _finishMode = (finishMode != 0);
      return S_OK;
    }

    void CDecoder::InitCommon() {
      {
        for(int i = 0; i < BCJ2_NUM_STREAMS; i++)
          dec.lims[i] = dec.bufs[i] = _bufs[i];
      }

      {
        for(int i = 0; i < BCJ2_NUM_STREAMS; i++) {
          _extraReadSizes[i] = 0;
          _inStreamsProcessed[i] = 0;
          _readRes[i] = S_OK;
        }
      }

      Bcj2Dec_Init(&dec);
    }

    HRESULT CDecoder::Code(ISequentialInStream * const *inStreams, const UInt64 * const *inSizes, UInt32 numInStreams,
                           ISequentialOutStream * const *outStreams, const UInt64 * const *outSizes, UInt32 numOutStreams,
                           ICompressProgressInfo *progress) {
      if(numInStreams != BCJ2_NUM_STREAMS || numOutStreams != 1)
        return E_INVALIDARG;

      RINOK(Alloc());

      InitCommon();

      dec.destLim = dec.dest = _bufs[BCJ2_NUM_STREAMS];

      UInt64 outSizeProcessed = 0;
      UInt64 prevProgress = 0;

      HRESULT res = S_OK;

      for(;;) {
        if(Bcj2Dec_Decode(&dec) != SZ_OK)
          return S_FALSE;

        if(dec.state < BCJ2_NUM_STREAMS) {
          size_t totalRead = _extraReadSizes[dec.state];
          {
            Byte *buf = _bufs[dec.state];
            for(size_t i = 0; i < totalRead; i++)
              buf[i] = dec.bufs[dec.state][i];
            dec.lims[dec.state] =
              dec.bufs[dec.state] = buf;
          }

          if(_readRes[dec.state] != S_OK) {
            res = _readRes[dec.state];
            break;
          }

          do {
            UInt32 curSize = _bufsCurSizes[dec.state] - (UInt32)totalRead;
            /*
            we want to call Read even even if size is 0
            if (inSizes && inSizes[dec.state])
            {
              UInt64 rem = *inSizes[dec.state] - _inStreamsProcessed[dec.state];
              if (curSize > rem)
                curSize = (UInt32)rem;
            }
            */

            HRESULT res2 = inStreams[dec.state]->Read(_bufs[dec.state] + totalRead, curSize, &curSize);
            _readRes[dec.state] = res2;
            if(curSize == 0)
              break;
            _inStreamsProcessed[dec.state] += curSize;
            totalRead += curSize;
            if(res2 != S_OK)
              break;
          } while(totalRead < 4 && BCJ2_IS_32BIT_STREAM(dec.state));

          if(_readRes[dec.state] != S_OK)
            res = _readRes[dec.state];

          if(totalRead == 0)
            break;

          // res == S_OK;

          if(BCJ2_IS_32BIT_STREAM(dec.state)) {
            unsigned extraSize = ((unsigned)totalRead & 3);
            _extraReadSizes[dec.state] = extraSize;
            if(totalRead < 4) {
              res = (_readRes[dec.state] != S_OK) ? _readRes[dec.state] : S_FALSE;
              break;
            }
            totalRead -= extraSize;
          }

          dec.lims[dec.state] = _bufs[dec.state] + totalRead;
        } else // if (dec.state <= BCJ2_STATE_ORIG)
        {
          size_t curSize = dec.dest - _bufs[BCJ2_NUM_STREAMS];
          if(curSize != 0) {
            outSizeProcessed += curSize;
            RINOK(WriteStream(outStreams[0], _bufs[BCJ2_NUM_STREAMS], curSize));
          }
          dec.dest = _bufs[BCJ2_NUM_STREAMS];
          {
            size_t rem = _bufsCurSizes[BCJ2_NUM_STREAMS];
            if(outSizes && outSizes[0]) {
              UInt64 outSize = *outSizes[0] - outSizeProcessed;
              if(rem > outSize)
                rem = (size_t)outSize;
            }
            dec.destLim = dec.dest + rem;
            if(rem == 0)
              break;
          }
        }

        if(progress) {
          UInt64 outSize2 = outSizeProcessed + (dec.dest - _bufs[BCJ2_NUM_STREAMS]);
          if(outSize2 - prevProgress >= (1 << 22)) {
            UInt64 inSize2 = outSize2 + _inStreamsProcessed[BCJ2_STREAM_RC] - (dec.lims[BCJ2_STREAM_RC] - dec.bufs[BCJ2_STREAM_RC]);
            RINOK(progress->SetRatioInfo(&inSize2, &outSize2));
            prevProgress = outSize2;
          }
        }
      }

      size_t curSize = dec.dest - _bufs[BCJ2_NUM_STREAMS];
      if(curSize != 0) {
        outSizeProcessed += curSize;
        RINOK(WriteStream(outStreams[0], _bufs[BCJ2_NUM_STREAMS], curSize));
      }

      if(res != S_OK)
        return res;

      if(_finishMode) {
        if(!Bcj2Dec_IsFinished(&dec))
          return S_FALSE;

        // we still allow the cases when input streams are larger than required for decoding.
        // so the case (dec.state == BCJ2_STATE_ORIG) is also allowed, if MAIN stream is larger than required.
        if(dec.state != BCJ2_STREAM_MAIN &&
           dec.state != BCJ2_DEC_STATE_ORIG)
          return S_FALSE;

        if(inSizes) {
          for(int i = 0; i < BCJ2_NUM_STREAMS; i++) {
            size_t rem = dec.lims[i] - dec.bufs[i] + _extraReadSizes[i];
            /*
            if (rem != 0)
              return S_FALSE;
            */
            if(inSizes[i] && *inSizes[i] != _inStreamsProcessed[i] - rem)
              return S_FALSE;
          }
        }
      }

      return S_OK;
    }

    STDMETHODIMP CDecoder::SetInStream2(UInt32 streamIndex, ISequentialInStream *inStream) {
      _inStreams[streamIndex] = inStream;
      return S_OK;
    }

    STDMETHODIMP CDecoder::ReleaseInStream2(UInt32 streamIndex) {
      _inStreams[streamIndex].Release();
      return S_OK;
    }

    STDMETHODIMP CDecoder::SetOutStreamSize(const UInt64 *outSize) {
      _outSizeDefined = (outSize != nullptr);
      _outSize = 0;
      if(_outSizeDefined)
        _outSize = *outSize;

      _outSize_Processed = 0;

      HRESULT res = Alloc(false);

      InitCommon();
      dec.destLim = dec.dest = nullptr;

      return res;
    }

    STDMETHODIMP CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize) {
      if(processedSize)
        *processedSize = 0;

      if(size == 0)
        return S_OK;

      UInt32 totalProcessed = 0;

      if(_outSizeDefined) {
        UInt64 rem = _outSize - _outSize_Processed;
        if(size > rem)
          size = (UInt32)rem;
      }
      dec.dest = (Byte *)data;
      dec.destLim = (const Byte *)data + size;

      HRESULT res = S_OK;

      for(;;) {
        SRes sres = Bcj2Dec_Decode(&dec);
        if(sres != SZ_OK)
          return S_FALSE;

        {
          UInt32 curSize = (UInt32)(dec.dest - (Byte *)data);
          if(curSize != 0) {
            totalProcessed += curSize;
            if(processedSize)
              *processedSize = totalProcessed;
            data = (void *)((Byte *)data + curSize);
            size -= curSize;
            _outSize_Processed += curSize;
          }
        }

        if(dec.state >= BCJ2_NUM_STREAMS)
          break;

        {
          size_t totalRead = _extraReadSizes[dec.state];
          {
            Byte *buf = _bufs[dec.state];
            for(size_t i = 0; i < totalRead; i++)
              buf[i] = dec.bufs[dec.state][i];
            dec.lims[dec.state] =
              dec.bufs[dec.state] = buf;
          }

          if(_readRes[dec.state] != S_OK)
            return _readRes[dec.state];

          do {
            UInt32 curSize = _bufsCurSizes[dec.state] - (UInt32)totalRead;
            HRESULT res2 = _inStreams[dec.state]->Read(_bufs[dec.state] + totalRead, curSize, &curSize);
            _readRes[dec.state] = res2;
            if(curSize == 0)
              break;
            _inStreamsProcessed[dec.state] += curSize;
            totalRead += curSize;
            if(res2 != S_OK)
              break;
          } while(totalRead < 4 && BCJ2_IS_32BIT_STREAM(dec.state));

          if(totalRead == 0) {
            if(totalProcessed == 0)
              res = _readRes[dec.state];
            break;
          }

          if(BCJ2_IS_32BIT_STREAM(dec.state)) {
            unsigned extraSize = ((unsigned)totalRead & 3);
            _extraReadSizes[dec.state] = extraSize;
            if(totalRead < 4) {
              if(totalProcessed != 0)
                return S_OK;
              return (_readRes[dec.state] != S_OK) ? _readRes[dec.state] : S_FALSE;
            }
            totalRead -= extraSize;
          }

          dec.lims[dec.state] = _bufs[dec.state] + totalRead;
        }
  }

      if(_finishMode && _outSizeDefined && _outSize == _outSize_Processed) {
        if(!Bcj2Dec_IsFinished(&dec))
          return S_FALSE;

        if(dec.state != BCJ2_STREAM_MAIN &&
           dec.state != BCJ2_DEC_STATE_ORIG)
          return S_FALSE;

        /*
        for (int i = 0; i < BCJ2_NUM_STREAMS; i++)
          if (dec.bufs[i] != dec.lims[i] || _extraReadSizes[i] != 0)
            return S_FALSE;
        */
      }

      return res;
}
  }
}