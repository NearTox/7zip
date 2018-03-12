// MultiStream.cpp

#include "../../../Common/Common.h"

#include "MultiStream.h"

STDMETHODIMP CMultiStream::Read(void *data, UInt32 size, UInt32 *processedSize) {
  if(processedSize)
    *processedSize = 0;
  if(size == 0)
    return S_OK;
  if(_pos >= _totalLength)
    return S_OK;

  {
    unsigned left = 0, mid = _streamIndex, right = Streams.Size();
    for(;;) {
      CSubStreamInfo &m = Streams[mid];
      if(_pos < m.GlobalOffset)
        right = mid;
      else if(_pos >= m.GlobalOffset + m.Size)
        left = mid + 1;
      else {
        _streamIndex = mid;
        break;
      }
      mid = (left + right) / 2;
    }
    _streamIndex = mid;
  }

  CSubStreamInfo &s = Streams[_streamIndex];
  UInt64 localPos = _pos - s.GlobalOffset;
  if(localPos != s.LocalPos) {
    RINOK(s.Stream->Seek(localPos, STREAM_SEEK_SET, &s.LocalPos));
  }
  UInt64 rem = s.Size - localPos;
  if(size > rem)
    size = (UInt32)rem;
  HRESULT result = s.Stream->Read(data, size, &size);
  _pos += size;
  s.LocalPos += size;
  if(processedSize)
    *processedSize = size;
  return result;
}

STDMETHODIMP CMultiStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) {
  switch(seekOrigin) {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _pos; break;
    case STREAM_SEEK_END: offset += _totalLength; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if(offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _pos = offset;
  if(newPosition)
    *newPosition = offset;
  return S_OK;
}