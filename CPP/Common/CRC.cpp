// Common/CRC.cpp

#include "../Common/Common.h"

#include "../../C/7zCrc.h"

struct CCRCTableInit {
  CCRCTableInit() { CrcGenerateTable(); }
} g_CRCTableInit;
