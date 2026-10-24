/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_OUTPUT_GRAPH_PROOF_H
#define PIONEER_Q38_GDN_OUTPUT_GRAPH_PROOF_H
#include "PioneerQ38GdnOutputGraphShape.h"
#include "PioneerQ38ProjectionGraphProof.h"
// Exact QOP1 transport DATA only. No synthetic input epoch/RMS producer
// admission. Native ownership, canonical pages, original snapshot, retained
// true gated producer and physical lifetime remain independently required.
static inline bool PioneerQ38GdnOutputGraphExpectedTraffic(const uint32_t* m,PioneerQ38ProjectionGraphTraffic& out){
 if(!PioneerQ38GdnOutputGraphFrame(m))return false;
 PioneerQ38ProjectionGraphTraffic t;
 if(m[5]==2){t.constantReads=7378;t.writes=t.writeFlits=238;t.minWaits=357;out=t;return true;}
 unsigned k=m[44],n=m[45],p=k/256,j=m[12],waves=m[7],chunks=(k*2+16383)/16384;
 unsigned scales=k>>(m[48]==8?5:8),scaleBytes=(scales*4+63)&~63U;
 unsigned scatter=0;
 for(unsigned wave=0;wave<waves;wave++){
  unsigned active=PioneerQ38ProjectionGraphWorkers(m,wave),row=m[192+wave*119*8+3],bytes=0;
  bool ordered=true;
  for(unsigned w=0;w<active;w++){
   const auto* r=m+192+(wave*119+w)*8;
   if(!r[2] || r[2]>16 || r[3]>=n || r[2]*16>n-r[3])return false;
   ordered &= r[3]==row;row=r[3]+r[2]*16;bytes+=r[2]*64;
  }
  scatter+=ordered?(bytes+16383)/16384:active;
 }
 t.constantReads=119*16+48*p+chunks+31*j+(chunks+1)*waves;
 t.writes=3*p+1+2*j+scatter;
 t.readExtraFlits=4*p+k/32-chunks+3*j+n/16+((k*2+scaleBytes)/64-chunks-1)*waves;
 t.writeFlits=6*p+scaleBytes/64+2*j+n/16;
 t.minWaits=119+2*p+j;out=t;return true;
}
static inline bool PioneerQ38GdnOutputGraphReceipt(const uint32_t* s,const uint32_t* m,const uint32_t* before,const uint32_t* after){
 PioneerQ38ProjectionGraphTraffic t;
 if(!s || !before || !after || !PioneerQ38GdnOutputGraphExpectedTraffic(m,t) || before[2]!=before[6] || after[2]!=after[6])return false;
 const bool work=m[5]==1;
 for(unsigned i=0;i<64;i++){
  uint32_t expected=0;
  if(i==0)expected=TT_Q38_GDN_OUTPUT_GRAPH_MARKER;
  else if(i==1)expected=1;
  else if(i==2 || i==61)expected=m[5];
  else if(i==6){
   if(s[10]<t.minWaits || uint64_t(s[10])>uint64_t(t.minWaits)*65536 || s[11]<t.minWaits || s[11]>s[10]
    || uint64_t(s[6])<uint64_t(s[10])+3*uint64_t(s[11])+t.constantReads
    || uint64_t(s[6])>4*uint64_t(s[10])+t.constantReads)return false;
   continue;
  }
  else if(i==7)expected=t.writes;
  else if(i==8){if(uint64_t(s[6])+t.readExtraFlits>UINT32_MAX)return false;expected=s[6]+t.readExtraFlits;}
  else if(i==9)expected=t.writeFlits;
  else if(i==10 || i==11)continue;
  else if(i==12 || i==54)expected=m[13];
  else if(i>=16 && i<24)expected=before[i-16];
  else if(i>=24 && i<32){unsigned c=i-24;
   uint32_t delta=c==0 || c==3?s[6]:c==1?s[8]:c==2 || c==6?s[6]+s[7]:c==5?s[9]:s[7];
   expected=before[c]+delta;if(after[c]!=expected)return false;
  }
  else if(i>=32 && i<48)expected=m[16+i-32];
  else if(i==48)expected=m[48];
  else if(i==49)expected=work?m[44]/256:0;
  else if(i==50)expected=work?m[7]:0;
  else if(i==51)expected=work?m[12]:0;
  else if(i==52)expected=m[44];
  else if(i==53)expected=m[45];
  else if(i==55)expected=m[42]|(m[43]<<8);
  else if(i==56)expected=1;
  else if(i==57 || i==58)expected=m[46+i-57];
  else if(i==59 || i==60)expected=m[40+i-59];
  if(s[i]!=expected)return false;
 }
 return true;
}
#endif
