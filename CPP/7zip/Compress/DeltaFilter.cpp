// DeltaFilter.cpp

#include "../../Common/Common.h"
#include "../../../C/Delta.h"

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/RegisterCodec.h"

namespace NCompress {
  namespace NDelta {
    struct CDelta {
      unsigned _delta;
      Byte _state[DELTA_STATE_SIZE];

      CDelta() : _delta(1) {}
      void DeltaInit() {
        Delta_Init(_state);
      }
    };

    class CDecoder :
      public ICompressFilter,
      public ICompressSetDecoderProperties2,
      CDelta,
      public CMyUnknownImp {
    public:
      MY_UNKNOWN_IMP2(ICompressFilter, ICompressSetDecoderProperties2)
        INTERFACE_ICompressFilter(;)
        STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);
    };

    STDMETHODIMP CDecoder::Init() {
      DeltaInit();
      return S_OK;
    }

    STDMETHODIMP_(UInt32) CDecoder::Filter(Byte *data, UInt32 size) {
      Delta_Decode(_state, _delta, data, size);
      return size;
    }

    STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *props, UInt32 size) {
      if(size != 1)
        return E_INVALIDARG;
      _delta = (unsigned)props[0] + 1;
      return S_OK;
    }

    REGISTER_FILTER_E(Delta, CDecoder(), 3, "Delta")
  }
}