// List.cpp

#include "../../../Common/Common.h"

#include "../../../Common/IntToString.h"
#include "../../../Common/MyCom.h"
#include "../../../Common/StdOutStream.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/UTFConvert.h"

#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/PropVariant.h"
#include "../../../Windows/PropVariantConv.h"

#include "../Common/OpenArchive.h"
#include "../Common/PropIDUtils.h"

#include "ConsoleClose.h"
#include "List.h"
#include "OpenCallbackConsole.h"

using namespace NWindows;
using namespace NCOM;

extern CStdOutStream* g_StdStream;
extern CStdOutStream* g_ErrStream;

static const char* const kPropIdToName[] = {
    "0",
    "1",
    "2",
    "Path",
    "Name",
    "Extension",
    "Folder",
    "Size",
    "Packed Size",
    "Attributes",
    "Created",
    "Accessed",
    "Modified",
    "Solid",
    "Commented",
    "Encrypted",
    "Split Before",
    "Split After",
    "Dictionary Size",
    "CRC",
    "Type",
    "Anti",
    "Method",
    "Host OS",
    "File System",
    "User",
    "Group",
    "Block",
    "Comment",
    "Position",
    "Path Prefix",
    "Folders",
    "Files",
    "Version",
    "Volume",
    "Multivolume",
    "Offset",
    "Links",
    "Blocks",
    "Volumes",
    "Time Type",
    "64-bit",
    "Big-endian",
    "CPU",
    "Physical Size",
    "Headers Size",
    "Checksum",
    "Characteristics",
    "Virtual Address",
    "ID",
    "Short Name",
    "Creator Application",
    "Sector Size",
    "Mode",
    "Symbolic Link",
    "Error",
    "Total Size",
    "Free Space",
    "Cluster Size",
    "Label",
    "Local Name",
    "Provider",
    "NT Security",
    "Alternate Stream",
    "Aux",
    "Deleted",
    "Tree",
    "SHA-1",
    "SHA-256",
    "Error Type",
    "Errors",
    "Errors",
    "Warnings",
    "Warning",
    "Streams",
    "Alternate Streams",
    "Alternate Streams Size",
    "Virtual Size",
    "Unpack Size",
    "Total Physical Size",
    "Volume Index",
    "SubType",
    "Short Comment",
    "Code Page",
    "Is not archive type",
    "Physical Size can't be detected",
    "Zeros Tail Is Allowed",
    "Tail Size",
    "Embedded Stub Size",
    "Link",
    "Hard Link",
    "iNode",
    "Stream ID",
    "Read-only",
    "Out Name",
    "Copy Link",
};

static const char kEmptyAttribChar = '.';

static const char* const kListing = "Listing archive: ";

static const char* const kString_Files = "files";
static const char* const kString_Dirs = "folders";
static const char* const kString_AltStreams = "alternate streams";
static const char* const kString_Streams = "streams";

static const char* const kError = "ERROR: ";

enum EAdjustment { kLeft, kCenter, kRight };

struct CFieldInfo {
  PROPID PropID;
  bool IsRawProp;
  UString NameU;
  AString NameA;
  EAdjustment TitleAdjustment;
  EAdjustment TextAdjustment;
  unsigned PrefixSpacesWidth;
  unsigned Width;
};

struct CFieldInfoInit {
  PROPID PropID;
  const char* Name;
  EAdjustment TitleAdjustment;
  EAdjustment TextAdjustment;
  unsigned PrefixSpacesWidth;
  unsigned Width;
};

static const CFieldInfoInit kStandardFieldTable[] = {
    {kpidMTime, "   Date      Time", kLeft, kLeft, 0, 19},
    {kpidAttrib, "Attr", kRight, kCenter, 1, 5},
    {kpidSize, "Size", kRight, kRight, 1, 12},
    {kpidPackSize, "Compressed", kRight, kRight, 1, 12},
    {kpidPath, "Name", kLeft, kLeft, 2, 24}};

const unsigned kNumSpacesMax = 32;  // it must be larger than max CFieldInfoInit.Width
static const char* g_Spaces = "                                ";

static void PrintSpaces(unsigned numSpaces) {
  if (numSpaces > 0 && numSpaces <= kNumSpacesMax)
    g_StdOut << g_Spaces + (kNumSpacesMax - numSpaces);
}

// extern int g_CodePage;

struct CListUInt64Def {
  UInt64 Val;
  bool Def;

  CListUInt64Def() : Val(0), Def(false) {}
  void Add(UInt64 v) {
    Val += v;
    Def = true;
  }
  void Add(const CListUInt64Def& v) {
    if (v.Def) Add(v.Val);
  }
};

struct CListFileTimeDef {
  FILETIME Val;
  bool Def;

  CListFileTimeDef() : Def(false) {
    Val.dwLowDateTime = 0;
    Val.dwHighDateTime = 0;
  }
  void Update(const CListFileTimeDef& t) {
    if (t.Def && (!Def || CompareFileTime(&Val, &t.Val) < 0)) {
      Val = t.Val;
      Def = true;
    }
  }
};

struct CListStat {
  CListUInt64Def Size;
  CListUInt64Def PackSize;
  CListFileTimeDef MTime;
  UInt64 NumFiles;

  CListStat() : NumFiles(0) {}
  void Update(const CListStat& st) {
    Size.Add(st.Size);
    PackSize.Add(st.PackSize);
    MTime.Update(st.MTime);
    NumFiles += st.NumFiles;
  }
  void SetSizeDefIfNoFiles() {
    if (NumFiles == 0) Size.Def = true;
  }
};

static void GetPropName(PROPID propID, const wchar_t* name, AString& nameA, UString& nameU) {
  if (propID < ARRAY_SIZE(kPropIdToName)) {
    nameA = kPropIdToName[propID];
    return;
  }
  if (name)
    nameU = name;
  else {
    nameA.Empty();
    nameA.Add_UInt32(propID);
  }
}

#ifndef _SFX

static inline char GetHex(Byte value) {
  return (char)((value < 10) ? ('0' + value) : ('A' + (value - 10)));
}

#endif

#define MY_ENDL endl

void Print_UInt64_and_String(AString& s, UInt64 val, const char* name);

static void PrintPropName_and_Eq(CStdOutStream& so, PROPID propID) {
  const char* s;
  char temp[16];
  if (propID < ARRAY_SIZE(kPropIdToName))
    s = kPropIdToName[propID];
  else {
    ConvertUInt32ToString(propID, temp);
    s = temp;
  }
  so << s << " = ";
}

static void PrintPropNameAndNumber(CStdOutStream& so, PROPID propID, UInt64 val) {
  PrintPropName_and_Eq(so, propID);
  so << val << endl;
}

static void PrintPropNameAndNumber_Signed(CStdOutStream& so, PROPID propID, Int64 val) {
  PrintPropName_and_Eq(so, propID);
  so << val << endl;
}

static void UString_Replace_CRLF_to_LF(UString& s) {
  // s.Replace(L"\r\n", L"\n");
  wchar_t* src = s.GetBuf();
  wchar_t* dest = src;
  for (;;) {
    wchar_t c = *src++;
    if (c == 0) break;
    if (c == '\r' && *src == '\n') {
      src++;
      c = '\n';
    }
    *dest++ = c;
  }
  s.ReleaseBuf_SetEnd((unsigned)(dest - s.GetBuf()));
}

static void PrintPropVal_MultiLine(CStdOutStream& so, const wchar_t* val) {
  UString s = val;
  if (s.Find(L'\n') >= 0) {
    so << endl;
    so << "{";
    so << endl;
    UString_Replace_CRLF_to_LF(s);
    so.Normalize_UString__LF_Allowed(s);
    so << s;
    so << endl;
    so << "}";
  } else {
    so.Normalize_UString(s);
    so << s;
  }
  so << endl;
}

static void PrintPropPair(CStdOutStream& so, const char* name, const wchar_t* val, bool multiLine) {
  so << name << " = ";
  if (multiLine) {
    PrintPropVal_MultiLine(so, val);
    return;
  }
  UString s = val;
  so.Normalize_UString(s);
  so << s;
  so << endl;
}

static void PrintPropertyPair2(
    CStdOutStream& so, PROPID propID, const wchar_t* name, const CPropVariant& prop) {
  UString s;
  ConvertPropertyToString2(s, prop, propID);
  if (!s.IsEmpty()) {
    AString nameA;
    UString nameU;
    GetPropName(propID, name, nameA, nameU);
    if (!nameA.IsEmpty())
      so << nameA;
    else
      so << nameU;
    so << " = ";
    PrintPropVal_MultiLine(so, s);
  }
}

static HRESULT PrintArcProp(
    CStdOutStream& so, IInArchive* archive, PROPID propID, const wchar_t* name) {
  CPropVariant prop;
  RINOK(archive->GetArchiveProperty(propID, &prop));
  PrintPropertyPair2(so, propID, name, prop);
  return S_OK;
}

static void PrintArcTypeError(CStdOutStream& so, const UString& type, bool isWarning) {
  so << "Open " << (isWarning ? "WARNING" : "ERROR") << ": Can not open the file as [" << type
     << "] archive" << endl;
}

int Find_FileName_InSortedVector(const UStringVector& fileName, const UString& name);

void PrintErrorFlags(CStdOutStream& so, const char* s, UInt32 errorFlags);

static void ErrorInfo_Print(CStdOutStream& so, const CArcErrorInfo& er) {
  PrintErrorFlags(so, "ERRORS:", er.GetErrorFlags());
  if (!er.ErrorMessage.IsEmpty()) PrintPropPair(so, "ERROR", er.ErrorMessage, true);

  PrintErrorFlags(so, "WARNINGS:", er.GetWarningFlags());
  if (!er.WarningMessage.IsEmpty()) PrintPropPair(so, "WARNING", er.WarningMessage, true);
}

HRESULT Print_OpenArchive_Props(
    CStdOutStream& so, const CCodecs* codecs, const CArchiveLink& arcLink) {
  FOR_VECTOR(r, arcLink.Arcs) {
    const CArc& arc = arcLink.Arcs[r];
    const CArcErrorInfo& er = arc.ErrorInfo;

    so << "--\n";
    PrintPropPair(so, "Path", arc.Path, false);
    if (er.ErrorFormatIndex >= 0) {
      if (er.ErrorFormatIndex == arc.FormatIndex)
        so << "Warning: The archive is open with offset" << endl;
      else
        PrintArcTypeError(so, codecs->GetFormatNamePtr(er.ErrorFormatIndex), true);
    }
    PrintPropPair(so, "Type", codecs->GetFormatNamePtr(arc.FormatIndex), false);

    ErrorInfo_Print(so, er);

    Int64 offset = arc.GetGlobalOffset();
    if (offset != 0) PrintPropNameAndNumber_Signed(so, kpidOffset, offset);
    IInArchive* archive = arc.Archive;
    RINOK(PrintArcProp(so, archive, kpidPhySize, nullptr));
    if (er.TailSize != 0) PrintPropNameAndNumber(so, kpidTailSize, er.TailSize);
    {
      UInt32 numProps;
      RINOK(archive->GetNumberOfArchiveProperties(&numProps));

      for (UInt32 j = 0; j < numProps; j++) {
        CMyComBSTR name;
        PROPID propID;
        VARTYPE vt;
        RINOK(archive->GetArchivePropertyInfo(j, &name, &propID, &vt));
        RINOK(PrintArcProp(so, archive, propID, name));
      }
    }

    if (r != arcLink.Arcs.Size() - 1) {
      UInt32 numProps;
      so << "----\n";
      if (archive->GetNumberOfProperties(&numProps) == S_OK) {
        UInt32 mainIndex = arcLink.Arcs[r + 1].SubfileIndex;
        for (UInt32 j = 0; j < numProps; j++) {
          CMyComBSTR name;
          PROPID propID;
          VARTYPE vt;
          RINOK(archive->GetPropertyInfo(j, &name, &propID, &vt));
          CPropVariant prop;
          RINOK(archive->GetProperty(mainIndex, propID, &prop));
          PrintPropertyPair2(so, propID, name, prop);
        }
      }
    }
  }
  return S_OK;
}

HRESULT Print_OpenArchive_Error(
    CStdOutStream& so, const CCodecs* codecs, const CArchiveLink& arcLink) {
#ifndef _NO_CRYPTO
  if (arcLink.PasswordWasAsked)
    so << "Can not open encrypted archive. Wrong password?";
  else
#endif
  {
    if (arcLink.NonOpen_ErrorInfo.ErrorFormatIndex >= 0) {
      so.NormalizePrint_UString(arcLink.NonOpen_ArcPath);
      so << endl;
      PrintArcTypeError(
          so, codecs->Formats[arcLink.NonOpen_ErrorInfo.ErrorFormatIndex].Name, false);
    } else
      so << "Can not open the file as archive";
  }

  so << endl;
  so << endl;
  ErrorInfo_Print(so, arcLink.NonOpen_ErrorInfo);

  return S_OK;
}
