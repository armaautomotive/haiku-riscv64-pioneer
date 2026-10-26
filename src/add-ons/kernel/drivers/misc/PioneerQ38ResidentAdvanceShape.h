/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_ADVANCE_SHAPE_H
#define PIONEER_Q38_RESIDENT_ADVANCE_SHAPE_H
#include "PioneerQ38ResidentLayerLinkShape.h"
#include "PioneerQ38ProjectionResultLayout.h"
// NEW on-card next-layer projection schedule DATA, not native58/69
// ownership. The additional L1 region below is an OFFLINE geometry only:
// a future native owner owes exclusive reservation, arbitrary-original
// snapshots, complete source/clock/host lifetime and proved restoration.
enum {TT_Q38_ADVANCE_MARKER=0x31504151,TT_Q38_ADVANCE_FAULT=0x46504151,
 TT_Q38_ADVANCE_RECEIPT_MARKER=0x31524151,TT_Q38_ADVANCE_ROLE_WORDS=1152,
 TT_Q38_ADVANCE_WORDS=64+4*TT_Q38_ADVANCE_ROLE_WORDS,
 TT_Q38_ADVANCE_MATRIX_ROM=0x40000,TT_Q38_ADVANCE_ROM=0x46000,TT_Q38_ADVANCE_GATHER_ROM=0x4e000,
 TT_Q38_ADVANCE_MANIFEST=0x50000,TT_Q38_ADVANCE_SHADOW=0x55000,
 TT_Q38_ADVANCE_RECEIPT=0x5a000,TT_Q38_ADVANCE_RECEIPT_WORDS=512,TT_Q38_ADVANCE_END=0x5b000};
static inline unsigned PioneerQ38ResidentAdvanceRole(unsigned i){return i==0?10:i==1?11:i==2?5:i==3?6:~0U;}
static inline const uint32_t* PioneerQ38ResidentAdvanceRecord(const uint32_t* d,unsigned i){return d && i<4?d+64+i*TT_Q38_ADVANCE_ROLE_WORDS:nullptr;}
static inline bool PioneerQ38ResidentAdvanceShape(const uint32_t* d,const uint32_t* link,const uint32_t* cohort){
 if(!d || !link || !cohort || link[0]!=TT_Q38_LAYER_LINK_MARKER || !link[4] || link[4]>=64 || link[4]%4==3
  || link[10]>=0x7ffffff8 || d[0]!=TT_Q38_ADVANCE_MARKER || d[1]!=1 || d[2]!=TT_Q38_ADVANCE_WORDS
  || d[3]!=link[4] || d[4]!=link[5] || d[5]!=link[6] || d[6]!=link[7] || d[7]!=link[8]
  || d[8]!=link[10] || d[9]!=d[8]+1 || d[10]!=d[8]+2 || d[11]!=d[8]+5
  || d[12]!=4 || d[13]!=20 || d[14]!=244 || d[15]!=PioneerQ38ProjectionRoute(d[6])
  || d[40]!=TT_Q38_LAYER_LINK_RECEIPT_MARKER || d[41]!=8 || d[42]!=5 || d[43]!=TT_Q38_RESIDENT_COHORT_MARKER)return false;
 for(unsigned i=16;i<40;i++)if(d[i]!=link[i] || d[i]!=cohort[i])return false;
 for(unsigned i=44;i<64;i++)if(d[i])return false;
 for(unsigned role=0;role<4;role++){
  const auto* r=PioneerQ38ResidentAdvanceRecord(d,role);unsigned semantic=PioneerQ38ResidentAdvanceRole(role),rows=PioneerQ38ProjectionResultRows(semantic);
  if(r[0]!=semantic || r[1]!=8 || r[2]!=(role<2?3U:119U) || r[3]!=5120 || r[4]!=rows || r[5]!=d[10]+role
   || r[6]!=PioneerQ38ProjectionResultOffset(semantic) || r[7]!=rows*4)return false;
  for(unsigned i=8;i<64;i++)if(r[i])return false;
  for(unsigned i=1135;i<1152;i++)if(r[i])return false;
  unsigned total=0;
  for(unsigned w=0;w<119;w++){
   const auto* p=r+64+w*8;unsigned first=r[1016+w];
   if(w>=r[2]){for(unsigned i=0;i<8;i++)if(p[i])return false;if(first)return false;continue;}
   unsigned bytes=p[2]*92160;
   if(p[0]<0x04000000 || p[0]>=0xc4000000 || (p[0]&63) || p[1]>=7 || !p[2] || p[2]>16 || p[3]
    || bytes>0xc4000000-p[0] || p[4]!=PioneerQ38ProjectionRoute(p[1]) || !p[5]
    || p[5]!=PioneerQ38CohortSource(cohort,w+1)[0] || p[6] || p[7]
    || (first&15) || first>=rows || p[2]*16>rows-first
    || (p[1]==d[6] && p[0]<d[7]+67108864 && d[7]<p[0]+bytes))return false;
   for(unsigned old=0;old<w;old++){
    const auto* a=r+64+old*8;
    if((first<r[1016+old]+a[2]*16 && r[1016+old]<first+p[2]*16)
     || (p[1]==a[1] && p[0]<a[0]+a[2]*92160 && a[0]<p[0]+bytes))return false;
   }
   total+=p[2]*16;
  }
  if(total!=rows)return false;
 }
 return true; // complete geometry, NEVER canonical/authenticated producer readiness
}
static inline bool PioneerQ38ResidentAdvanceMatrixReceipt(const uint32_t* s,const uint32_t* d,const uint32_t* r,
 const uint32_t* before,const uint32_t* after){
 if(!s || !d || !r || !before || !after || before[2]!=before[6] || after[2]!=after[6]
  || s[10]<r[2] || s[10]>r[2]*65536U || s[11]<r[2] || s[11]>s[10]
  || s[57]!=0x31435951 || !(s[58]|s[59]) || !(s[60]|s[61]) || !(s[62]|s[63]))return false;
 unsigned groups=r[4]/16,row=r[1016];bool ordered=true;
 for(unsigned w=0;w<r[2];w++){ordered &= r[1016+w]==row;row=r[1016+w]+r[64+w*8+2]*16;}
 unsigned scatter=ordered?(r[7]+16383)/16384:r[2];
 unsigned reads=s[10]+3*s[11]+r[2]+2,flits=s[10]+3*s[11]+4*r[2]+groups+170,writes=2*r[2]+scatter,writeFlits=2*r[2]+groups;
 for(unsigned i=0;i<64;i++){
  unsigned expected=0;
  if(i==0 || i==49)expected=0x314d5051;
  else if(i==1 || i==2 || i==48 || i==56)expected=1;
  else if(i==3)expected=8;else if(i==4)expected=5120;else if(i==5)expected=r[2];
  else if(i==6)expected=reads;else if(i==7)expected=writes;else if(i==8)expected=flits;else if(i==9)expected=writeFlits;
  else if(i==10 || i==11 || i>=57)continue;
  else if(i==12)expected=r[5];else if(i>=16 && i<24)expected=before[i-16];else if(i>=24 && i<32)expected=after[i-24];
  else if(i>=32 && i<48)expected=d[16+i-32];else if(i==50)expected=2;else if(i==51)expected=170;
  else if(i==52)expected=scatter;else if(i==53)expected=groups;else if(i==55)expected=r[4];
  if(s[i]!=expected)return false;
 }
 for(unsigned i=0;i<8;i++){
  unsigned delta=i==0 || i==3?reads:i==1?flits:i==2 || i==6?reads+writes:i==5?writeFlits:writes;
  if(uint32_t(after[i]-before[i])!=delta)return false;
 }
 return true; // DATA only, not a native or numeric validation capability
}
static inline bool PioneerQ38ResidentAdvanceTraffic(const uint32_t* r){
 if(!r || r[48]<647 || r[48]>647*65536U || r[49]!=647 || r[50]!=20 || r[51]!=244 || r[52]!=4 || r[53]!=119
  || r[44]<15548+r[48]+3*647 || r[44]>15548+4*r[48] || r[45]!=60 || r[46]!=r[44]+60 || r[47]!=120)return false;
 unsigned reads=r[44]+r[64],writes=r[45]+r[66],flits=r[46]+r[65],writeFlits=r[47]+r[67];
 for(unsigned role=0;role<4;role++){
  const auto* s=r+96+role*64;reads+=s[6];writes+=s[7];flits+=s[8];writeFlits+=s[9];
 }
 for(unsigned i=0;i<8;i++){
  unsigned delta=i==0 || i==3?reads:i==1?flits:i==2 || i==6?reads+writes:i==5?writeFlits:writes;
  if(uint32_t(r[360+i]-r[352+i])!=delta)return false;
 }
 return true; // necessary complete traffic DATA, never IO/terminal/publication authority
}
static inline bool PioneerQ38ResidentAdvanceReceipt(const uint32_t* r,const uint32_t* d,const uint32_t* link,const uint32_t* cohort,
 const uint32_t* before,const uint32_t* after,const uint32_t* command){
 if(!r || !before || !after || !command || !PioneerQ38ResidentAdvanceShape(d,link,cohort) || !PioneerQ38ResidentAdvanceTraffic(r))return false;
 for(unsigned i=0;i<44;i++)if(r[i]!=(i==0?unsigned(TT_Q38_ADVANCE_RECEIPT_MARKER):i==2?unsigned(TT_Q38_ADVANCE_RECEIPT_WORDS):d[i]))return false;
 for(unsigned i=54;i<64;i++)if(r[i])return false;
 const auto* g=r+64;const unsigned summary[]={21,200,1,10,20,10240,d[9],0x31475051};
 const unsigned fields[]={d[6],d[7],d[3],5,8,640,5120,129},delta[]={21,200,22,21,1,10,22,1};
 for(unsigned i=0;i<8;i++)if(g[i]!=summary[i] || g[24+i]!=fields[i] || uint32_t(g[16+i]-g[8+i])!=delta[i]
  || r[352+i]!=before[i] || r[360+i]!=after[i])return false;
 for(unsigned role=0;role<4;role++){
  const auto* s=r+96+role*64;
  if(!PioneerQ38ResidentAdvanceMatrixReceipt(s,d,PioneerQ38ResidentAdvanceRecord(d,role),s+16,s+24))return false;
 }
 const uint32_t last[]={0xffb20228,0,PioneerQ38ConvPeer(119),0x75008,0,129,0,0,4,0};
 for(unsigned i=0;i<10;i++)if(r[368+i]!=last[i] || command[i]!=last[i])return false;
 for(unsigned w=1;w<120;w++){
  unsigned initial=cohort[64+w-1];
  if(initial>=0x7fffffe0)return false;
  unsigned layer=initial+5+unsigned(w<=96)+unsigned(w<=24)+unsigned(w<=20)+unsigned(w<=68);
  unsigned next=layer+unsigned(w<=20)+2+2*unsigned(w<=3);if(r[377+w]!=next)return false;
 }
 for(unsigned i=497;i<512;i++)if(r[i])return false;
 return true; // no numeric/token/state-slot/native lifetime or cleanup authority
}
static_assert(TT_Q38_ADVANCE_MATRIX_ROM+0x6000==TT_Q38_ADVANCE_ROM
 && TT_Q38_ADVANCE_ROM+0x8000==TT_Q38_ADVANCE_GATHER_ROM
 && TT_Q38_ADVANCE_GATHER_ROM+0x2000==TT_Q38_ADVANCE_MANIFEST
 && TT_Q38_ADVANCE_MANIFEST+TT_Q38_ADVANCE_WORDS*4<=TT_Q38_ADVANCE_SHADOW
 && TT_Q38_ADVANCE_SHADOW+TT_Q38_ADVANCE_WORDS*4<=TT_Q38_ADVANCE_RECEIPT
 && TT_Q38_ADVANCE_RECEIPT+TT_Q38_ADVANCE_RECEIPT_WORDS*4<=TT_Q38_ADVANCE_END
 && TT_Q38_ADVANCE_END<0x64000,"NEW unqualified L1 region; NO old journal/view/ownership grant");
#endif
