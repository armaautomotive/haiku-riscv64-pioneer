/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GLOBAL_FEEDBACK_LAYOUT_H
#define PIONEER_Q38_GLOBAL_FEEDBACK_LAYOUT_H
#include "PioneerQ38GlobalChainLayout.h"
#include "PioneerQ38RecurrentHistoryLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingShape.h"
// Geometry ONLY. The new native owner must still authenticate/initialize the
// original embedding stripes and reserve this provisional workspace output.
// A decoded embedding is NOT a KV/GDN/Conv slot flip or token commit.
namespace PioneerQ38GlobalFeedbackLayout {
namespace C=PioneerQ38GlobalChainLayout;
enum {Magic=0x31464551,Offset=0x700000,Bytes=20480,Receipt=C::Head+512,
 Tile=2304,Blocks=20,Values=5120,Local=C::R::Projected,Raw=0x90000,Shadow=C::R::Final};
static inline bool Describe(unsigned token,unsigned position,unsigned bank,unsigned workspace,uint32_t* output){
 if(!output || (reinterpret_cast<__UINTPTR_TYPE__>(output)&3)
  || reinterpret_cast<__UINTPTR_TYPE__>(output)>~static_cast<__UINTPTR_TYPE__>(0)-32 || token>=248320 || position>=8192 || bank>=7
  || workspace<0x04000000 || workspace>0xc0000000 || (workspace&4095))return false;
 unsigned source=0;while(source<7 && (token<PioneerQ38EmbeddingFirst(source)
  || token-PioneerQ38EmbeddingFirst(source)>=PioneerQ38EmbeddingRows(source)))source++;
 if(source>=7)return false;
 unsigned row=token-PioneerQ38EmbeddingFirst(source);
 const unsigned stage[]={token,position,bank,workspace+Offset,source,0x04000000+(row/16)*46080,row&15,20};
 if(source==bank && stage[5]<workspace+C::WorkspaceBytes && workspace<stage[5]+46080)return false;
 for(unsigned i=0;i<8;i++)output[i]=stage[i];
 return true;
}
static_assert(Offset>0x600000+Bytes && Offset+Bytes<=PioneerQ38RecurrentHistoryLayout::Offset,
 "provisional next embedding cannot overwrite initial hidden, staged Conv histories or packages");
static_assert(Receipt+256<=unsigned(C::R::Projected) && C::Head+C::HeadBytes<=Receipt,
 "distinct feedback diagnostic, not the candidate/STOP/parts/header record");
static_assert(Local+Bytes<=Raw && Raw+Tile<=unsigned(C::R::Native),"closed-phase numeric scratch remains disjoint");
}
#endif
