/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_PROOF_H
#define PIONEER_Q38_RESIDENT_LAYER_PROOF_H
#include "PioneerQ38ResidentLayerShape.h"
#include "PioneerQ38ResidentOutputProof.h"
#include "PioneerQ38FfnResidualGraphProof.h"
// NEW full-layer completion DATA,never source/IO/cleanup or native authority.
// The actual owner owes independent physical BEFORE/AFTER,exact installed
// code/immutable manifests/model/source/clock and unheld ROOT+peer terminals.
static inline bool PioneerQ38ResidentLayerRootTerminalPc(uint32_t pc){return pc==0x72038 || pc==0x7203c;}
static inline bool PioneerQ38ResidentLayerRootRejectedPc(uint32_t pc){return pc>=0x72040 && pc<0x72080;}
static inline bool PioneerQ38ResidentLayerRootPhysical(const uint32_t* status,const uint32_t* pc,uint32_t reset){
 return status && pc && reset==0x46800 && status[0]==0x7200c && status[1]==5120 && status[2]==TT_Q38_RESIDENT_LAYER_MARKER
  && PioneerQ38ResidentLayerRootTerminalPc(pc[0]) && PioneerQ38ResidentLayerRootTerminalPc(pc[1]);
}
static inline unsigned PioneerQ38ResidentLayerSequence(const uint32_t* m,unsigned w){
 unsigned prefix=PioneerQ38ResidentOutputSequence(m,w);
 if(prefix==UINT32_MAX || prefix>0x7ffffff8)return UINT32_MAX;
 return prefix+3+unsigned(w<=20)+unsigned(w<=68);
}
static inline bool PioneerQ38ResidentLayerRootReceipt(const uint32_t* l,const uint32_t* m,const uint32_t* q,
 const uint32_t* r,const uint32_t* live,const uint32_t* f,const uint32_t* s,const uint32_t* before,const uint32_t* after,const uint32_t* command){
 if(!r || !live || !f || !s || !before || !after || !command || !PioneerQ38ResidentLayerShape(l,m,q)
  || !PioneerQ38FfnResidualGraphManifest(f,m+TT_Q38_COHORT_SOURCE_WORD+16))return false;
 const uint32_t header[]={TT_Q38_RESIDENT_LAYER_RECEIPT_MARKER,2,TT_Q38_RESIDENT_LAYER_RECEIPT_WORDS,l[3],l[4],l[11],l[5],l[6],l[7],l[8],l[9],l[10],TT_Q38_RESIDENT_LAYER_MARKER,5120,3,1};
 for(unsigned i=0;i<16;i++)if(r[i]!=header[i] || r[16+i]!=l[16+i])return false;
 for(unsigned i=0;i<8;i++)if(r[224+i]!=l[32+i] || r[232+i]!=before[i])return false;
 if(r[178]!=68 || r[179]!=32 || r[180]!=TT_Q38_RESIDENT_LAYER_GDN_SAVED)return false;
 for(unsigned i=181;i<192;i++)if(r[i])return false;
 for(unsigned i=240;i<256;i++)if(r[i])return false;
 // These are the actual saved earlier-prefix data,not a retagged old
 // CLOSED owner or reconstructed receipt. Independent BEFORE is mandatory.
 if(!PioneerQ38ResidentOutputRootReceipt(m,q,before,r+160,r+32,r+96,r+168))return false;
 for(unsigned i=0;i<64;i++){
  uint32_t expected=i<10?r[32+i]:i==10 || i==12?5120:i==11?10:i==13?3:i==14?68:i==15?20:i==16?9:i==17?uint32_t(TT_Q38_RESIDENT_LAYER_MARKER):0;
  if(live[i]!=expected)return false;
 }
 for(unsigned i=0;i<TT_Q38_FFN_RESIDUAL_GRAPH_WORDS;i++){
  unsigned expected=i>=64 && i<183?PioneerQ38ResidentOutputSequence(m,i-63):l[64+i];
  if(f[i]!=expected)return false;
 }
 const uint32_t normHeader[]={6,960,4,640,5120,20480,l[6],TT_Q38_RESIDENT_LAYER_NORM_MARKER};
 const unsigned delta[]={6,960,10,6,4,640,10,4};
 for(unsigned i=0;i<8;i++)if(r[192+i]!=normHeader[i] || r[200+i]!=r[160+i] || r[208+i]!=s[16+i]
  || uint32_t(r[208+i]-r[200+i])!=delta[i])return false;
 const uint32_t normReceipt[]={l[3],l[9],l[11],l[4],l[12],l[13],l[44],98304};
 for(unsigned i=0;i<8;i++)if(r[216+i]!=normReceipt[i])return false;
 if(!PioneerQ38FfnResidualGraphReceipt(s,f,r+208,after))return false;
 const uint32_t last[]={0x9d000,0,129,l[9]+16384,0,l[10],0,0x212,4096,0};
 for(unsigned i=0;i<10;i++)if(command[i]!=last[i])return false;
 return true; // NO current-slot/token publication or reset/restore permission
}
static inline bool PioneerQ38ResidentLayerPeerPhysical(const uint32_t* m,unsigned w,const uint32_t* source,
 const uint32_t* boot,const uint32_t* status,uint32_t doorbell,const uint32_t* pc,uint32_t reset,bool stop){
 if(!m || !w || w>=120 || !source || !boot || !status || !pc || reset!=0x46800)return false;
 unsigned previous=PioneerQ38ResidentLayerSequence(m,w);if(previous==UINT32_MAX || previous>=0x7ffffffe)return false;
 unsigned seq=previous+unsigned(stop),route=PioneerQ38ConvPeer(w);
 for(unsigned i=0;i<16;i++)if(source[i]!=PioneerQ38CohortSource(m,w)[i]
  || status[i]!=(i<2?seq:i==3?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==4?unsigned(stop)*2:0))return false;
 for(unsigned i=0;i<8;i++)if(boot[i]!=(i==0?unsigned(TT_Q38_FFN_PERSISTENT_MARKER):i==1?w:i==2?route:i==3?source[0]:0))return false;
 for(unsigned i=0;i<2;i++)if(stop?!PioneerQ38PeerTerminalPc(pc[i]):pc[i]!=0x7204c && pc[i]!=0x72050)return false;
 return doorbell==seq;
}
static inline bool PioneerQ38ResidentLayerPeerReceipt(const uint32_t* l,const uint32_t* m,const uint32_t* q,unsigned w,
 const uint32_t* before,const uint32_t* after,const uint32_t* command,const uint32_t* proof,const uint32_t* outputBefore,
 const uint32_t* downBefore,const uint32_t* first,const uint32_t* recurrent,const uint32_t* gated,const uint32_t* clock){
 if(!l || !m || !q || !w || w>=120 || !before || !after || !command || !proof || !outputBefore || !downBefore
  || before[2]!=before[6] || after[2]!=after[6])return false;
 uint32_t tracked[8];unsigned n=PioneerQ38ConvBatchCount(w);
 const unsigned conv[]={3*n,8*n,5*n,3*n,2*n,4*n,5*n,2*n};
 for(unsigned i=0;i<8;i++)tracked[i]=before[i]+conv[i];
 if(w<=96){
  const auto* in=PioneerQ38CohortGdn(m,w);const auto* st=in+64;const auto* ga=st+64;
  if(!first || !recurrent || !gated || !clock || !PioneerQ38GdnInputSummary(first,in,tracked,first+16)
   || !PioneerQ38ResidentGdnSummary(recurrent,st,first+16,recurrent+16)
   || !PioneerQ38GdnRendezvousSummary(gated,ga,in,st,recurrent+16,gated+16)
   || !PioneerQ38GdnRendezvousClockReceipt(clock,ga,gated[5]))return false;
  for(unsigned i=0;i<8;i++)tracked[i]=gated[16+i];
 }
 if(w<=24){const unsigned delta[]={1,16,3,1,2,10,3,2};for(unsigned i=0;i<8;i++)tracked[i]+=delta[i];}
 for(unsigned i=0;i<8;i++)if(outputBefore[i]!=tracked[i])return false;
 const auto* out=q+TT_Q38_OUTPUT_COHORT_PART+(w-1)*8;
 if(!out[2] || out[2]>16)return false;
 for(unsigned i=0;i<8;i++)tracked[i]+=PioneerQ38ResidentOutputDelta(i,out[2]*192+2,0,out[2]*1728+204,0);
 if(w<=20)for(unsigned i=0;i<8;i++)tracked[i]+=PioneerQ38ResidentOutputDelta(i,1,2,16,9);
 const auto* f=l+64;
 for(unsigned role=0;role<2;role++){
  const auto* r=f+PioneerQ38FfnResidualGraphRoleOffset(role);const auto* p=r+16+(w-1)*8;
  if(r[2]!=119 || !p[2] || p[2]>16 || r[3]!=5120 || (r[1]!=12 && r[1]!=14))return false;
  unsigned reads=p[2]*20+2,flits=p[2]*20*(r[1]==12?36U:53U)+162;
  for(unsigned i=0;i<8;i++)tracked[i]+=PioneerQ38ResidentOutputDelta(i,reads,0,flits,0);
 }
 if(w<=68)for(unsigned i=0;i<8;i++)tracked[i]+=PioneerQ38ResidentOutputDelta(i,2,2,32,9);
 const auto* r=f+PioneerQ38FfnResidualGraphRoleOffset(2);const auto* p=r+16+(w-1)*8;
 unsigned groups=p[2],sequence=PioneerQ38ResidentLayerSequence(m,w);
 if(r[2]!=119 || !groups || groups>16 || r[3]!=17408 || (r[1]!=12 && r[1]!=14) || sequence==UINT32_MAX)return false;
 unsigned tile=r[1]==12?2304:3392,reads=groups*68+4,flits=groups*68*(tile/64)+549;
 for(unsigned i=0;i<8;i++)if(downBefore[i]!=tracked[i] || proof[i]!=tracked[i] || proof[8+i]!=after[i]
  || uint32_t(after[i]-tracked[i])!=PioneerQ38ResidentOutputDelta(i,reads,0,flits,0))return false;
 if(proof[16]!=groups*1088 || proof[17]!=sequence || proof[18]!=w || proof[19]!=TT_Q38_FFN_PERSISTENT_MARKER
  || proof[30]!=PioneerQ38CohortSource(m,w)[0] || proof[31]!=PioneerQ38ConvPeer(w))return false;
 const uint32_t last[]={p[0]+(groups*68-1)*tile,0,p[4],0x90000,0,PioneerQ38ConvPeer(w),0,0,tile,0};
 for(unsigned i=0;i<10;i++)if(proof[20+i]!=last[i] || command[i]!=last[i])return false;
 for(unsigned i=0;i<16;i++)if(proof[32+i]!=PioneerQ38CohortSource(m,w)[i])return false;
 for(unsigned i=48;i<64;i++)if(proof[i])return false;
 return true; // full physical counter lineage,not numeric/IO/lifetime authority
}
#endif
