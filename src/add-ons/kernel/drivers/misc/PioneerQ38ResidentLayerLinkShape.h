/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_LINK_SHAPE_H
#define PIONEER_Q38_RESIDENT_LAYER_LINK_SHAPE_H
#include "PioneerQ38ResidentLayerShape.h"
#include "PioneerQ38LearnedRmsShape.h"
// Device-to-device hidden-state handoff DATA. A descriptor/receipt is NOT
// a completed layer, native owner, initialized page or execution grant.
// Only the NEW linked program may consume the output of its own actual
// layer call. Neither old native58 nor the qualified native69 selects it.
enum {TT_Q38_LAYER_LINK_MARKER=0x314b4c51,TT_Q38_LAYER_LINK_FAULT=0x464b4c51,
 TT_Q38_LAYER_LINK_RECEIPT_MARKER=0x31504b51,TT_Q38_LAYER_LINK_WORDS=128,
 TT_Q38_LAYER_LINK_MANIFEST=0x17e000,TT_Q38_LAYER_LINK_SHADOW=0x17e200,
 TT_Q38_LAYER_LINK_RECEIPT=0x17e400,TT_Q38_LAYER_LINK_RECEIPT_WORDS=96,
 TT_Q38_LAYER_LINK_ROM=0x159600,TT_Q38_LAYER_LINK_ROM_END=0x15c000};
static inline bool PioneerQ38ResidentLayerLinkShape(const uint32_t* d,const uint32_t* l){
 if(!d || !l || l[0]!=TT_Q38_RESIDENT_LAYER_MARKER || l[1]!=1 || l[2]!=TT_Q38_RESIDENT_LAYER_WORDS
  || l[3]>=63 || l[3]%4==3 || l[4]>=8192 || l[11]>=248320 || l[7]>=0x7ffffff8
  || d[0]!=TT_Q38_LAYER_LINK_MARKER || d[1]!=1 || d[2]!=TT_Q38_LAYER_LINK_WORDS
  || d[3]!=l[3] || d[4]!=l[3]+1 || d[5]!=l[4] || d[6]!=l[11]
  || d[7]!=l[8] || d[8]!=l[9] || d[9]!=l[7]+5 || d[10]!=d[9]+1
  || d[11]!=5120 || d[12]!=TT_Q38_RESIDENT_LAYER_RECEIPT_MARKER
  || d[13]!=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER || d[14]!=4 || d[15]!=4)return false;
 for(unsigned i=16;i<40;i++)if(d[i]!=l[i])return false;
 for(unsigned i=40;i<64;i++)if(d[i])return false;
 const auto* n=d+64;
 if(!PioneerQ38LearnedRmsShape(n) || n[3]!=d[7] || n[4]!=d[8] || n[5]!=d[4]
  || n[6]!=d[10] || n[7] || n[40]!=d[5])return false;
 for(unsigned i=16;i<40;i++)if(n[i]!=d[i])return false;
 return true;
}
static inline bool PioneerQ38ResidentLayerLinkReceipt(const uint32_t* r,const uint32_t* d,const uint32_t* l,
 const uint32_t* before,const uint32_t* after,const uint32_t* command){
 if(!r || !before || !after || !command || !PioneerQ38ResidentLayerLinkShape(d,l))return false;
 for(unsigned i=0;i<16;i++)if(r[i]!=(i==0?unsigned(TT_Q38_LAYER_LINK_RECEIPT_MARKER):i==2?unsigned(TT_Q38_LAYER_LINK_RECEIPT_WORDS):d[i]))return false;
 for(unsigned i=16;i<40;i++)if(r[i]!=d[i])return false;
 if(!PioneerQ38LearnedRmsSummary(r+40,d+64,before,after))return false;
 const uint32_t last[]={0x94000,0,129,d[8]+16384,0,PioneerQ38ProjectionRoute(d[7]),0,0x212,4096,0};
 for(unsigned i=0;i<10;i++)if(r[72+i]!=last[i] || command[i]!=last[i])return false;
 for(unsigned i=82;i<TT_Q38_LAYER_LINK_RECEIPT_WORDS;i++)if(r[i])return false;
 return true; // no token/state commit, STOP, hold, restore or END authority
}
static_assert(TT_Q38_RESIDENT_LAYER_NORM_ROM+5404<=TT_Q38_LAYER_LINK_ROM
 && unsigned(TT_Q38_LAYER_LINK_ROM_END)==unsigned(TT_Q38_RESIDENT_LAYER_FFN_ROM)
 && unsigned(TT_Q38_RESIDENT_LAYER_ROM_END)==unsigned(TT_Q38_LAYER_LINK_MANIFEST)
 && TT_Q38_LAYER_LINK_MANIFEST+TT_Q38_LAYER_LINK_WORDS*4==TT_Q38_LAYER_LINK_SHADOW
 && TT_Q38_LAYER_LINK_SHADOW+TT_Q38_LAYER_LINK_WORDS*4==TT_Q38_LAYER_LINK_RECEIPT
 && TT_Q38_LAYER_LINK_RECEIPT+TT_Q38_LAYER_LINK_RECEIPT_WORDS*4<0x17ffc0,
 "NEW link footprint; never an expanded native69 mapping/restore grant");
#endif
