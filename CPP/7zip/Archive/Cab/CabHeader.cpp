// CabHeader.cpp

#include "../../../Common/Common.h"

#include "CabHeader.h"

namespace NArchive {
  namespace NCab {
    namespace NHeader {
      const Byte kMarker[kMarkerSize] = {'M', 'S', 'C', 'F', 0, 0, 0, 0};

      // struct CSignatureInitializer { CSignatureInitializer() { kMarker[0]--; } } g_SignatureInitializer;
    }
  }
}