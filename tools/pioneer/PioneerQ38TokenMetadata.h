/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_METADATA_H
#define PIONEER_Q38_TOKEN_METADATA_H
#include "PioneerQ38GlobalFeedbackLayout.h"
#include "PioneerQ38DensePartitionLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnRendezvousShape.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BlockFfnRecipe.h"
// DATA transforms from the immutable position-zero canonical templates.
// No initialization, producer, native lease, state commit or launch grant.
namespace PioneerQ38TokenMetadata {
namespace C=PioneerQ38GlobalChainLayout;namespace R=C::R;namespace D=C::D;
static inline unsigned Phase(unsigned position,unsigned layer){return 1001+position*1024+layer*16;}
static inline unsigned Request(unsigned position,unsigned layer){return position*64+layer+1;}
static inline bool Context(unsigned layer,unsigned position,unsigned token){return layer<64 && position<8192 && token<248320;}
static inline bool Frame(const uint32_t* d,unsigned layer,unsigned position,unsigned token,unsigned bank,unsigned workspace,const uint32_t* boot){
 if(!Context(layer,position,token) || !C::Frame(boot,d,C::Dense(layer)) || d[3]!=layer || d[4]!=position
  || d[6]!=Phase(position,layer) || d[7]!=bank || d[9]!=workspace || d[48]!=Request(position,layer))return false;
 return C::Dense(layer)?d[52]==token && d[73]==bank && d[74]==workspace+R::FinalOffset:
  d[12]==(position&1) && d[13]==token && d[68]==bank
  && d[69]==workspace+(layer?unsigned(R::FinalOffset):unsigned(PioneerQ38GlobalFeedbackLayout::Offset));
}
static __attribute__((noinline)) bool Outer(uint32_t* d,unsigned layer,unsigned position,unsigned token,unsigned bank,unsigned workspace,const uint32_t* boot){
 if(!Context(layer,position,token) || !C::LayerFrame(d,layer,bank,workspace,boot))return false;
 uint32_t stage[128];for(unsigned i=0;i<128;i++)stage[i]=d[i];
 stage[4]=position;stage[6]=Phase(position,layer);stage[48]=Request(position,layer);
 if(C::Dense(layer))stage[52]=token;
 else {stage[12]=position&1;stage[13]=token;stage[68]=bank;stage[69]=workspace+(layer?unsigned(R::FinalOffset):unsigned(PioneerQ38GlobalFeedbackLayout::Offset));}
 if(!Frame(stage,layer,position,token,bank,workspace,boot))return false;
 for(unsigned i=0;i<128;i++)d[i]=stage[i];
 return true;
}
static inline bool RecipeShape(const uint32_t* d,const uint32_t* h,const uint32_t* r,unsigned layer,unsigned slot){
 const auto* banks=r+1151;const auto* parts=reinterpret_cast<const uint32_t(*)[8]>(r+80);const auto* rows=r+1032;
 return !C::Dense(layer)?PioneerQ38MixedRecurrentRecipe(d,h,banks,parts,rows):slot<4?
  PioneerQ38DenseProjectionRecipe(d,h,banks,parts,rows,false):PioneerQ38BlockFfnRecipe(d,h,banks,parts,rows);
}
static __attribute__((noinline)) bool Recipe(uint32_t* r,unsigned layer,unsigned slot,unsigned position,unsigned token){
 if(!r || !Context(layer,position,token) || slot>=(C::Dense(layer)?7U:8U))return false;
 unsigned offset=C::Dense(layer)?slot+(slot>=4):R::PhaseDelta(slot);
 unsigned role=C::Dense(layer)?slot<3?5+slot:slot==3?10:slot-2:R::Role(slot);
 if(r[40] || r[41] || r[42]!=layer || r[43]!=role || r[6]!=Phase(0,layer)+offset || r[70]!=r[6]
  || !RecipeShape(r,r+64,r,layer,slot))return false;
 for(unsigned i=48;i<64;i++)if(r[i])return false;
 for(unsigned i=1159;i<1280;i++)if(r[i])return false;
 uint32_t d[64],h[16];for(unsigned i=0;i<64;i++)d[i]=r[i];for(unsigned i=0;i<16;i++)h[i]=r[64+i];
 d[6]=h[6]=Phase(position,layer)+offset;d[40]=position;d[41]=token;
 if(!RecipeShape(d,h,r,layer,slot))return false;
 r[6]=d[6];r[40]=position;r[41]=token;r[70]=h[6];return true;
}
static __attribute__((noinline)) bool Conv(uint32_t* d,unsigned layer,unsigned worker,unsigned slot,unsigned position){
 if(!d || layer>=64 || C::Dense(layer) || worker>=120 || slot>=6 || position>=8192)return false;
 if(slot>=PioneerQ38ConvBatchCount(worker)){for(unsigned i=0;i<64;i++)if(d[i])return false;return true;}
 if(!PioneerQ38ConvShape(d) || d[5]!=layer || d[10]!=worker || d[7]!=(worker+slot*120)*16
  || d[40] || d[6]!=Phase(0,layer)+4)return false;
 uint32_t stage[64];for(unsigned i=0;i<64;i++)stage[i]=d[i];stage[6]=Phase(position,layer)+4;stage[40]=position;
 if(!PioneerQ38ConvShape(stage))return false;
 d[6]=stage[6];d[40]=position;return true;
}
static __attribute__((noinline)) bool Gdn(uint32_t* d,unsigned layer,unsigned worker,unsigned position){
 if(!d || layer>=64 || C::Dense(layer) || worker>=96 || position>=8192 || d[5]!=layer || d[7]!=worker
  || d[40] || d[6]!=Phase(0,layer)+5 || d[64+11] || d[128+41]
  || !PioneerQ38GdnRecurrentShape(d,d+64) || !PioneerQ38GdnRendezvousShape(d+128,d,d+64))return false;
 uint32_t stage[192];for(unsigned i=0;i<192;i++)stage[i]=d[i];auto* s=stage+64;auto* g=stage+128;
 stage[6]=s[8]=g[6]=Phase(position,layer)+5;stage[40]=s[40]=g[40]=position;s[11]=g[41]=position&1;
 if(position&1){
  for(unsigned i=4;i<6;i++){unsigned swap=s[i];s[i]=s[i+2];s[i+2]=swap;}
  unsigned swap=s[12];s[12]=s[13];s[13]=swap;
 }
 if(!PioneerQ38GdnRecurrentShape(stage,s) || !PioneerQ38GdnRendezvousShape(g,stage,s))return false;
 for(unsigned i=0;i<192;i++)d[i]=stage[i];
 return true;
}
static __attribute__((noinline)) bool Dense(uint32_t* d,unsigned layer,unsigned worker,unsigned position){
 namespace W=PioneerQ38DensePartitionLayout;
 if(!d || layer>=64 || !C::Dense(layer) || worker>=96 || position>=8192 || !W::Shape(d)
  || d[3]!=layer || d[4] || d[15]!=worker+1 || d[40]!=Phase(0,layer)+3)return false;
 uint32_t stage[64];for(unsigned i=0;i<64;i++)stage[i]=d[i];
 stage[4]=position;stage[7]=position+1;stage[40]=Phase(position,layer)+3;
 unsigned per=W::Per(position+1),first=per*stage[6],count=first>=position+1?0:position+1-first;
 if(count>per)count=per;
 stage[48]=first;stage[49]=count;
 unsigned prior=first>=position?0:position-first;if(prior>count)prior=count;
 stage[46]=2*((prior+31)>>5);stage[47]=prior*16;
 if(!W::Shape(stage))return false;
 for(unsigned i=0;i<64;i++)d[i]=stage[i];
 return true;
}
}
#endif
