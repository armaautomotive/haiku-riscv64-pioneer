/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_Q8_MATRIX_WORKSPACE_H
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_WORKSPACE_H
#if !defined(PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Recurrent matrix workspace layout is offline inspection only
#endif
#include "PioneerQ38RecurrentQ8MatrixManifest.h"
#include "PioneerQ38TokenLayout.h"
#include <array>

// Proposed 128-byte per-projection card-DRAM records, not native reservation,
// authenticated upload, or device ownership. Records deliberately avoid a
// 12 KiB persistent L1 manifest that would overlap currently live ROOT data.
struct PioneerQ38RecurrentQ8MatrixWorkspace {
 using M=PioneerQ38RecurrentQ8MatrixManifest;
 enum : unsigned {Offset=0x3000200,RecordBytes=128,RecordWords=RecordBytes/4,
  Records=M::Layers*M::Roles,Bytes=Records*RecordBytes,Words=Bytes/4};
 static_assert(Offset>=PioneerQ38TokenLayout::C::HeadOffset+PioneerQ38TokenLayout::C::HeadBytes
  && Offset+Bytes<=PioneerQ38TokenLayout::SessionOffset && !(Offset&63) && !(RecordBytes&63),
  "experimental recurrent records must not overlap head or token session");
 static bool Build(const M::Canonical& source,const M::Plan::Proposal& proposal,uint32_t* output,unsigned words){
  if(!output || words!=Words || (reinterpret_cast<uintptr_t>(output)&3))return false;
  const uintptr_t dst=reinterpret_cast<uintptr_t>(output),src=reinterpret_cast<uintptr_t>(&source);
  const uintptr_t own=reinterpret_cast<uintptr_t>(&proposal),parts=reinterpret_cast<uintptr_t>(proposal.parts.data());
  if(proposal.parts.size()>SIZE_MAX/sizeof(proposal.parts[0]))return false;
  const size_t partBytes=proposal.parts.size()*sizeof(proposal.parts[0]);
  if(dst>UINTPTR_MAX-Bytes || src>UINTPTR_MAX-sizeof(source) || own>UINTPTR_MAX-sizeof(proposal) || parts>UINTPTR_MAX-partBytes
   || (dst<src+sizeof(source) && src<dst+Bytes) || (dst<own+sizeof(proposal) && own<dst+Bytes)
   || (partBytes && dst<parts+partBytes && parts<dst+Bytes))return false;
  std::array<uint32_t,M::Words> manifest={};
  if(!M::Build(source,proposal,manifest.data(),manifest.size()))return false;
  std::array<uint32_t,Words> stage={};
  for(unsigned r=0;r<Records;r++)for(unsigned i=0;i<21;i++)stage[r*RecordWords+i]=manifest[r*21+i];
  for(unsigned i=0;i<Words;i++)output[i]=stage[i];return true;
 }
};
#endif
