// RegisterArc.h

#ifndef __REGISTER_ARC_H
#define __REGISTER_ARC_H

#include "../Archive/IArchive.h"

struct CArcInfo {
  UInt16 Flags;
  Byte Id;
  Byte SignatureSize;
  UInt16 SignatureOffset;

  const Byte *Signature;
  const char *Name;
  const char *Ext;
  const char *AddExt;

  Func_CreateInArchive CreateInArchive;
  Func_IsArc IsArc;

  bool IsMultiSignature() const {
    return (Flags & NArcInfoFlags::kMultiSignature) != 0;
  }
};

void RegisterArc(const CArcInfo *arcInfo) throw();

#define IMP_CreateArcIn_2(c) static IInArchive *CreateArc() { return new c; }

#define IMP_CreateArcIn IMP_CreateArcIn_2(CHandler())

#define REGISTER_ARC_V(n, e, ae, id, sigSize, sig, offs, flags, crIn, isArc) \
  static const CArcInfo g_ArcInfo = { flags, id, sigSize, offs, sig, n, e, ae, crIn, isArc } ; \

#define REGISTER_ARC_R(n, e, ae, id, sigSize, sig, offs, flags, crIn, isArc) \
  REGISTER_ARC_V(n, e, ae, id, sigSize, sig, offs, flags, crIn, isArc) \
  struct CRegisterArc { CRegisterArc() { RegisterArc(&g_ArcInfo); }}; \
  static CRegisterArc g_RegisterArc;

#define REGISTER_ARC_I_CLS(cls, n, e, ae, id, sig, offs, flags, isArc) \
  IMP_CreateArcIn_2(cls) \
  REGISTER_ARC_R(n, e, ae, id, ARRAY_SIZE(sig), sig, offs, flags, CreateArc, isArc)

#define REGISTER_ARC_I_CLS_NO_SIG(cls, n, e, ae, id, offs, flags, isArc) \
  IMP_CreateArcIn_2(cls) \
  REGISTER_ARC_R(n, e, ae, id, 0, nullptr, offs, flags, CreateArc, isArc)

#define REGISTER_ARC_I(n, e, ae, id, sig, offs, flags, isArc) \
  REGISTER_ARC_I_CLS(CHandler(), n, e, ae, id, sig, offs, flags, isArc)

#define REGISTER_ARC_I_NO_SIG(n, e, ae, id, offs, flags, isArc) \
  REGISTER_ARC_I_CLS_NO_SIG(CHandler(), n, e, ae, id, offs, flags, isArc)

#define REGISTER_ARC_IO(n, e, ae, id, sig, offs, flags, isArc) \
  IMP_CreateArcIn \
  REGISTER_ARC_R(n, e, ae, id, ARRAY_SIZE(sig), sig, offs, flags, CreateArc, isArc)

#define REGISTER_ARC_IO_DECREMENT_SIG(n, e, ae, id, sig, offs, flags, isArc) \
  IMP_CreateArcIn \
  REGISTER_ARC_V(n, e, ae, id, ARRAY_SIZE(sig), sig, offs, flags, CreateArc, isArc) \
  struct CRegisterArcDecSig { CRegisterArcDecSig() { sig[0]--; RegisterArc(&g_ArcInfo); }}; \
  static CRegisterArcDecSig g_RegisterArc;

#endif