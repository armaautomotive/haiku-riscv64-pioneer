/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_GATED_SHAPE_H
#define PIONEER_Q38_GDN_GATED_SHAPE_H
#include "PioneerQ38GdnRecurrentShape.h"
// OFFLINE DATA only. Neither canonical addresses nor matching epoch words
// grant an uploaded source,an initialized owner,a release or a publication.
enum {TT_Q38_GDN_GATED_MARKER=0x31484751,TT_Q38_GDN_GATED_FAULT=0x46484751,
 TT_Q38_GDN_GATED_DESCRIPTOR=0x74a00,TT_Q38_GDN_GATED_SUMMARY=0x74b00,
 TT_Q38_GDN_GATED_READY=0x74c00,TT_Q38_GDN_GATED_PEER_READY=0x74c40,
 TT_Q38_GDN_GATED_OUTPUT=0x150000,TT_Q38_GDN_GATED_POLLS=256};
static inline bool PioneerQ38GdnGatedShape(const uint32_t* d,const uint32_t* input,const uint32_t* state){
 if(!d || !PioneerQ38GdnRecurrentShape(input,state) || d[0]!=TT_Q38_GDN_GATED_MARKER || d[1]!=1
  || d[2]!=input[2] || d[3]!=input[3] || d[4]!=input[4] || d[5]!=input[5] || d[6]!=input[6]
  || d[7]!=input[7] || d[8]!=input[8] || d[9]!=input[9] || d[10]!=input[10]
  || d[11]>=7 || (d[12]&4095) || d[12]<0x04000000 || d[12]>0xc3fff000
  || d[13]!=PioneerQ38ProjectionRoute(d[11]) || d[14]!=((d[7]^1)+1) || d[15]!=PioneerQ38ConvPeer(d[14])
  || d[40]!=input[40] || d[41]!=state[11]
  || d[42]!=PioneerQ38ProjectionResultOffset(5)+(d[7]>>1)*512
  || d[43]!=TT_Q38_GDN_GATED_OUTPUT+d[7]*256 || d[44]!=TT_Q38_GDN_GATED_READY || d[45]!=0x8e800
  || d[46]!=128 || d[47]!=64 || d[48]!=512 || d[49]!=512 || d[50]!=256 || d[51]!=TT_Q38_GDN_GATED_POLLS)return false;
 for(unsigned i=16;i<40;i++)if(d[i]!=input[i])return false;
 for(unsigned i=52;i<64;i++)if(d[i])return false;
 // The shared learned page must not alias workspace,CURRENT or NEXT.
 if(!PioneerQ38GdnRecurrentSeparate(d[11],d[12],4096,input[3],input[4],67108864))return false;
 for(unsigned side=0;side<2;side++)if(!PioneerQ38GdnRecurrentSeparate(d[11],d[12],4096,state[4+side*2],state[5+side*2],32768))return false;
 return true;
}
static inline uint32_t PioneerQ38GdnGatedReadyWord(const uint32_t* d,unsigned i,bool peer){
 if(!i)return TT_Q38_GDN_GATED_MARKER;
 if(i==1)return d[6];
 if(i==2)return d[7]^unsigned(peer);
 if(i==3)return d[5];
 if(i==4)return d[40];
 if(i==5)return d[41];
 if(i<14)return d[32+i-6];
 return i==14?128:64;
}
static inline bool PioneerQ38GdnGatedSummary(const uint32_t* s,const uint32_t* d,const uint32_t* input,
 const uint32_t* state,const uint32_t* before,const uint32_t* after){
 if(!s || !before || !after || !PioneerQ38GdnGatedShape(d,input,state)
  || !s[5] || s[5]>TT_Q38_GDN_GATED_POLLS || before[2]!=before[6] || after[2]!=after[6])return false;
 unsigned polls=s[5];const unsigned first[]={polls+3,polls+20,1,4,64,polls,d[6],TT_Q38_GDN_GATED_MARKER};
 const unsigned delta[]={polls+3,polls+20,polls+4,polls+3,1,4,polls+4,1};
 for(unsigned i=0;i<8;i++)if(s[i]!=first[i] || s[8+i]!=before[i] || s[16+i]!=after[i] || uint32_t(after[i]-before[i])!=delta[i])return false;
 const unsigned tail[]={d[7],d[5],d[40],d[15],d[4]+d[43],d[3],d[11],d[12]};
 for(unsigned i=0;i<8;i++)if(s[24+i]!=tail[i])return false;
 return true;
}
static_assert(TT_Q38_CONV_NEXT+30720*4<=TT_Q38_GDN_GATED_OUTPUT
 && TT_Q38_GDN_GATED_OUTPUT+6144*4<=2097152,"private gated output disjoint inside existing workspace prefix");
#endif
