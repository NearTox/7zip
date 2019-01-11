// HashCon.cpp

#include "../../../Common/Common.h"

#include "../../../Common/IntToString.h"

#include "ConsoleClose.h"
#include "HashCon.h"

static const char* const kEmptyFileAlias = "[Content]";

static const char* const kScanningMessage = "Scanning";

void Print_DirItemsStat(AString& s, const CDirItemsStat& st);

static void AddSpaces_if_Positive(AString& s, int num) {
  for (int i = 0; i < num; i++) s.Add_Space();
}

static const unsigned kSizeField_Len = 13;
static const unsigned kNameField_Len = 12;

static const unsigned kHashColumnWidth_Min = 4 * 2;

static const char* const k_DigestTitles[] = {
    " : ", " for data:              ", " for data and names:    ", " for streams and names: "};

static void PrintSum(CStdOutStream& so, const CHasherState& h, unsigned digestIndex) {
  so << h.Name;

  {
    AString temp;
    AddSpaces_if_Positive(temp, 6 - (int)h.Name.Len());
    so << temp;
  }

  so << k_DigestTitles[digestIndex];

  char s[k_HashCalc_DigestSize_Max * 2 + 64];
  s[0] = 0;
  AddHashHexToString(s, h.Digests[digestIndex], h.DigestSize);
  so << s << endl;
}

void PrintHashStat(CStdOutStream& so, const CHashBundle& hb) {
  FOR_VECTOR(i, hb.Hashers) {
    const CHasherState& h = hb.Hashers[i];
    PrintSum(so, h, k_HashCalc_Index_DataSum);
    if (hb.NumFiles != 1 || hb.NumDirs != 0) PrintSum(so, h, k_HashCalc_Index_NamesSum);
    if (hb.NumAltStreams != 0) PrintSum(so, h, k_HashCalc_Index_StreamsSum);
    so << endl;
  }
}
