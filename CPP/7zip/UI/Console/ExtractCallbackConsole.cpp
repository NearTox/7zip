// ExtractCallbackConsole.cpp

#include "../../../Common/Common.h"

#include "../../../Common/IntToString.h"
#include "../../../Common/Wildcard.h"

#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileFind.h"
#include "../../../Windows/TimeUtils.h"
#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/PropVariantConv.h"

#ifndef _7ZIP_ST
#include "../../../Windows/Synchronization.h"
#endif

#include "../Common/ExtractingFilePath.h"

#include "ExtractCallbackConsole.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;
static const char *kError = "ERROR: ";

HRESULT CExtractScanConsole::ScanProgress(const CDirItemsStat&, const FString&, bool /* isDir */) {
  return S_OK;
}

HRESULT CExtractScanConsole::ScanError(const FString &path, DWORD systemError) {
  ClosePercentsAndFlush();

  if(_se) {
    *_se << endl << kError << NError::MyFormatMessage(systemError) << endl <<
      fs2us(path) << endl << endl;
    _se->Flush();
  }
  return HRESULT_FROM_WIN32(systemError);
}

void Print_UInt64_and_String(AString &s, UInt64 val, const char *name) {
  char temp[32];
  ConvertUInt64ToString(val, temp);
  s += temp;
  s.Add_Space();
  s += name;
}

void PrintSize_bytes_Smart(AString &s, UInt64 val) {
  Print_UInt64_and_String(s, val, "bytes");

  if(val == 0)
    return;

  unsigned numBits = 10;
  char c = 'K';
  char temp[4] = {'K', 'i', 'B', 0};
  if(val >= ((UInt64)10 << 30)) {
    numBits = 30; c = 'G';
  } else if(val >= ((UInt64)10 << 20)) {
    numBits = 20; c = 'M';
  }
  temp[0] = c;
  s += " (";
  Print_UInt64_and_String(s, ((val + ((UInt64)1 << numBits) - 1) >> numBits), temp);
  s += ')';
}

void Print_DirItemsStat(AString &s, const CDirItemsStat &st) {
  if(st.NumDirs != 0) {
    Print_UInt64_and_String(s, st.NumDirs, st.NumDirs == 1 ? "folder" : "folders");
    s += ", ";
  }
  Print_UInt64_and_String(s, st.NumFiles, st.NumFiles == 1 ? "file" : "files");
  s += ", ";
  PrintSize_bytes_Smart(s, st.FilesSize);
  if(st.NumAltStreams != 0) {
    s.Add_LF();
    Print_UInt64_and_String(s, st.NumAltStreams, "alternate streams");
    s += ", ";
    PrintSize_bytes_Smart(s, st.AltStreamsSize);
  }
}

void CExtractScanConsole::PrintStat(const CDirItemsStat &st) {
  if(_so) {
    AString s;
    Print_DirItemsStat(s, st);
    *_so << s << endl;
  }
}

#ifndef _7ZIP_ST
static NSynchronization::CCriticalSection g_CriticalSection;
#define MT_LOCK NSynchronization::CCriticalSectionLock lock(g_CriticalSection);
#else
#define MT_LOCK
#endif

static const char *kTestString = "T";
static const char *kExtractString = "-";
static const char *kSkipString = ".";

// static const char *kCantAutoRename = "can not create file with auto name\n";
// static const char *kCantRenameFile = "can not rename existing file\n";
// static const char *kCantDeleteOutputFile = "can not delete output file ";

static const char *kMemoryExceptionMessage = "Can't allocate required memory!";

static const char *kExtracting = "Extracting archive: ";
static const char *kTesting = "Testing archive: ";

static const char *kEverythingIsOk = "Everything is Ok";
static const char *kNoFiles = "No files to process";

static const char *kUnsupportedMethod = "Unsupported Method";
static const char *kCrcFailed = "CRC Failed";
static const char *kCrcFailedEncrypted = "CRC Failed in encrypted file. Wrong password?";
static const char *kDataError = "Data Error";
static const char *kDataErrorEncrypted = "Data Error in encrypted file. Wrong password?";
static const char *kUnavailableData = "Unavailable data";
static const char *kUnexpectedEnd = "Unexpected end of data";
static const char *kDataAfterEnd = "There are some data after the end of the payload data";
static const char *kIsNotArc = "Is not archive";
static const char *kHeadersError = "Headers Error";
static const char *kWrongPassword = "Wrong password";

static const char * const k_ErrorFlagsMessages[] =
{
  "Is not archive"
  , "Headers Error"
  , "Headers Error in encrypted archive. Wrong password?"
  , "Unavailable start of archive"
  , "Unconfirmed start of archive"
  , "Unexpected end of archive"
  , "There are data after the end of archive"
  , "Unsupported method"
  , "Unsupported feature"
  , "Data Error"
  , "CRC Error"
};

STDMETHODIMP CExtractCallbackConsole::SetTotal(UInt64) {
  MT_LOCK
    return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::SetCompleted(const UInt64*) {
  MT_LOCK
    return S_OK;
}

static const char *kTab = "  ";

STDMETHODIMP CExtractCallbackConsole::PrepareOperation(const wchar_t *name, Int32, Int32 askExtractMode, const UInt64 *position) {
  MT_LOCK

    _currentName = name;

  const char *s;
  unsigned requiredLevel = 1;

  switch(askExtractMode) {
    case NArchive::NExtract::NAskMode::kExtract: s = kExtractString; break;
    case NArchive::NExtract::NAskMode::kTest:    s = kTestString; break;
    case NArchive::NExtract::NAskMode::kSkip:    s = kSkipString; requiredLevel = 2; break;
    default: s = "???"; requiredLevel = 2;
  };

  bool show2 = (0 >= requiredLevel && _so);

  if(show2) {
    _tempA = s;
    if(name)
      _tempA.Add_Space();
    *_so << _tempA;

    _tempU.Empty();
    if(name)
      _tempU = name;
    _so->PrintUString(_tempU, _tempA);
    if(position)
      *_so << " <" << *position << ">";
    *_so << endl;

    if(NeedFlush)
      _so->Flush();
  }
  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::MessageError(const wchar_t *message) {
  MT_LOCK

    RINOK(S_OK);

  NumFileErrors_in_Current++;
  NumFileErrors++;

  ClosePercentsAndFlush();
  if(_se) {
    *_se << kError << message << endl;
    _se->Flush();
  }

  return S_OK;
}

void SetExtractErrorMessage(Int32 opRes, Int32 encrypted, AString &dest) {
  dest.Empty();
  const char *s = nullptr;

  switch(opRes) {
    case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
      s = kUnsupportedMethod;
      break;
    case NArchive::NExtract::NOperationResult::kCRCError:
      s = (encrypted ? kCrcFailedEncrypted : kCrcFailed);
      break;
    case NArchive::NExtract::NOperationResult::kDataError:
      s = (encrypted ? kDataErrorEncrypted : kDataError);
      break;
    case NArchive::NExtract::NOperationResult::kUnavailable:
      s = kUnavailableData;
      break;
    case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
      s = kUnexpectedEnd;
      break;
    case NArchive::NExtract::NOperationResult::kDataAfterEnd:
      s = kDataAfterEnd;
      break;
    case NArchive::NExtract::NOperationResult::kIsNotArc:
      s = kIsNotArc;
      break;
    case NArchive::NExtract::NOperationResult::kHeadersError:
      s = kHeadersError;
      break;
    case NArchive::NExtract::NOperationResult::kWrongPassword:
      s = kWrongPassword;
      break;
  }

  dest += kError;
  if(s)
    dest += s;
  else {
    char temp[16];
    ConvertUInt32ToString(opRes, temp);
    dest += "Error #";
    dest += temp;
  }
}

STDMETHODIMP CExtractCallbackConsole::SetOperationResult(Int32 opRes, Int32 encrypted) {
  MT_LOCK

    if(opRes == NArchive::NExtract::NOperationResult::kOK) {
    } else {
      NumFileErrors_in_Current++;
      NumFileErrors++;

      if(_se) {
        ClosePercentsAndFlush();

        AString s;
        SetExtractErrorMessage(opRes, encrypted, s);

        *_se << s;
        if(!_currentName.IsEmpty())
          *_se << " : " << _currentName;
        *_se << endl;
        _se->Flush();
      }
    }

  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::ReportExtractResult(Int32 opRes, Int32 encrypted, const wchar_t *name) {
  if(opRes != NArchive::NExtract::NOperationResult::kOK) {
    _currentName = name;
    return SetOperationResult(opRes, encrypted);
  }

  return S_OK;
}

HRESULT CExtractCallbackConsole::SetPassword(const UString &password) {
  PasswordIsDefined = true;
  Password = password;
  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::CryptoGetTextPassword(BSTR *password) {
  COM_TRY_BEGIN
    MT_LOCK
    return Open_CryptoGetTextPassword(password);
  COM_TRY_END
}

HRESULT CExtractCallbackConsole::BeforeOpen(const wchar_t *name, bool testMode) {
  RINOK(S_OK);

  NumTryArcs++;
  ThereIsError_in_Current = false;
  ThereIsWarning_in_Current = false;
  NumFileErrors_in_Current = 0;
  if(_so)
    *_so << endl << (testMode ? kTesting : kExtracting) << name << endl;
  return S_OK;
}

HRESULT Print_OpenArchive_Props(CStdOutStream &so, const CCodecs *codecs, const CArchiveLink &arcLink);
HRESULT Print_OpenArchive_Error(CStdOutStream &so, const CCodecs *codecs, const CArchiveLink &arcLink);

static AString GetOpenArcErrorMessage(UInt32 errorFlags) {
  AString s;

  for(unsigned i = 0; i < ARRAY_SIZE(k_ErrorFlagsMessages); i++) {
    UInt32 f = (1 << i);
    if((errorFlags & f) == 0)
      continue;
    const char *m = k_ErrorFlagsMessages[i];
    if(!s.IsEmpty())
      s.Add_LF();
    s += m;
    errorFlags &= ~f;
  }

  if(errorFlags != 0) {
    char sz[16];
    sz[0] = '0';
    sz[1] = 'x';
    ConvertUInt32ToHex(errorFlags, sz + 2);
    if(!s.IsEmpty())
      s.Add_LF();
    s += sz;
  }

  return s;
}

void PrintErrorFlags(CStdOutStream &so, const char *s, UInt32 errorFlags) {
  if(errorFlags == 0)
    return;
  so << s << endl << GetOpenArcErrorMessage(errorFlags) << endl;
}

void Add_Messsage_Pre_ArcType(UString &s, const char *pre, const wchar_t *arcType) {
  s.Add_LF();
  s.AddAscii(pre);
  s.AddAscii(" as [");
  s += arcType;
  s.AddAscii("] archive");
}

void Print_ErrorFormatIndex_Warning(CStdOutStream *_so, const CCodecs *codecs, const CArc &arc) {
  const CArcErrorInfo &er = arc.ErrorInfo;

  UString s = L"WARNING:\n";
  s += arc.Path;
  if(arc.FormatIndex == er.ErrorFormatIndex) {
    s.Add_LF();
    s.AddAscii("The archive is open with offset");
  } else {
    Add_Messsage_Pre_ArcType(s, "Can not open the file", codecs->GetFormatNamePtr(er.ErrorFormatIndex));
    Add_Messsage_Pre_ArcType(s, "The file is open", codecs->GetFormatNamePtr(arc.FormatIndex));
  }

  *_so << s << endl << endl;
}

HRESULT CExtractCallbackConsole::OpenResult(const CCodecs *codecs, const CArchiveLink &arcLink, const wchar_t *name, HRESULT result) {
  ClosePercentsAndFlush();

  FOR_VECTOR(level, arcLink.Arcs) {
    const CArc &arc = arcLink.Arcs[level];
    const CArcErrorInfo &er = arc.ErrorInfo;

    UInt32 errorFlags = er.GetErrorFlags();

    if(errorFlags != 0 || !er.ErrorMessage.IsEmpty()) {
      if(_se) {
        *_se << endl;
        if(level != 0)
          *_se << arc.Path << endl;
      }

      if(errorFlags != 0) {
        if(_se)
          PrintErrorFlags(*_se, "ERRORS:", errorFlags);
        NumOpenArcErrors++;
        ThereIsError_in_Current = true;
      }

      if(!er.ErrorMessage.IsEmpty()) {
        if(_se)
          *_se << "ERRORS:" << endl << er.ErrorMessage << endl;
        NumOpenArcErrors++;
        ThereIsError_in_Current = true;
      }

      if(_se) {
        *_se << endl;
        _se->Flush();
      }
    }

    UInt32 warningFlags = er.GetWarningFlags();

    if(warningFlags != 0 || !er.WarningMessage.IsEmpty()) {
      if(_so) {
        *_so << endl;
        if(level != 0)
          *_so << arc.Path << endl;
      }

      if(warningFlags != 0) {
        if(_so)
          PrintErrorFlags(*_so, "WARNINGS:", warningFlags);
        NumOpenArcWarnings++;
        ThereIsWarning_in_Current = true;
      }

      if(!er.WarningMessage.IsEmpty()) {
        if(_so)
          *_so << "WARNINGS:" << endl << er.WarningMessage << endl;
        NumOpenArcWarnings++;
        ThereIsWarning_in_Current = true;
      }

      if(_so) {
        *_so << endl;
        if(NeedFlush)
          _so->Flush();
      }
    }

    if(er.ErrorFormatIndex >= 0) {
      if(_so) {
        Print_ErrorFormatIndex_Warning(_so, codecs, arc);
        if(NeedFlush)
          _so->Flush();
      }
      ThereIsWarning_in_Current = true;
    }
  }

  if(result == S_OK) {
    if(_so) {
      RINOK(Print_OpenArchive_Props(*_so, codecs, arcLink));
      *_so << endl;
    }
  } else {
    NumCantOpenArcs++;
    if(_so)
      _so->Flush();
    if(_se) {
      *_se << kError << name << endl;
      HRESULT res = Print_OpenArchive_Error(*_se, codecs, arcLink);
      RINOK(res);
      if(result == S_FALSE) {
      } else {
        if(result == E_OUTOFMEMORY)
          *_se << "Can't allocate required memory";
        else
          *_se << NError::MyFormatMessage(result);
        *_se << endl;
      }
      _se->Flush();
    }
  }

  return S_OK;
}

HRESULT CExtractCallbackConsole::ThereAreNoFiles() {
  if(_so) {
    *_so << endl << kNoFiles << endl;
    if(NeedFlush)
      _so->Flush();
  }
  return S_OK;
}

HRESULT CExtractCallbackConsole::ExtractResult(HRESULT result) {
  MT_LOCK
    if(_so)
      _so->Flush();

  if(result == S_OK) {
    if(NumFileErrors_in_Current == 0 && !ThereIsError_in_Current) {
      if(ThereIsWarning_in_Current)
        NumArcsWithWarnings++;
      else
        NumOkArcs++;
      if(_so)
        *_so << kEverythingIsOk << endl;
    } else {
      NumArcsWithError++;
      if(_so) {
        *_so << endl;
        if(NumFileErrors_in_Current != 0)
          *_so << "Sub items Errors: " << NumFileErrors_in_Current << endl;
      }
    }
    if(_so && NeedFlush)
      _so->Flush();
  } else {
    NumArcsWithError++;
    if(result == E_ABORT || result == ERROR_DISK_FULL)
      return result;

    if(_se) {
      *_se << endl << kError;
      if(result == E_OUTOFMEMORY)
        *_se << kMemoryExceptionMessage;
      else
        *_se << NError::MyFormatMessage(result);
      *_se << endl;
      _se->Flush();
    }
  }

  return S_OK;
}