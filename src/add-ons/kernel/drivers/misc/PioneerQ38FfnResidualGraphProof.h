/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_RESIDUAL_GRAPH_PROOF_H
#define PIONEER_Q38_FFN_RESIDUAL_GRAPH_PROOF_H
#include "PioneerQ38FfnResidualGraphShape.h"
// Exact fixed image transport DATA,not a native/producer/initialization
// grant. The actual owner supplies independent physical BEFORE and AFTER.
static inline unsigned PioneerQ38FfnResidualOutputPackets(const uint32_t* m,unsigned role){
 if(!m || role>=3)return 0;
 const auto* r=m+PioneerQ38FfnResidualGraphRoleOffset(role);unsigned next=0;
 for(unsigned w=0;w<r[2];w++){
  if(r[16+119*8+w]!=next)return r[2];
  next+=r[16+w*8+2]*16;
 }
 return (r[4]*4+16383)/16384;
}
static inline bool PioneerQ38FfnResidualGraphReceipt(const uint32_t* s,const uint32_t* m,const uint32_t* before,const uint32_t* after){
 if(!s || !before || !after || !PioneerQ38FfnResidualGraphFrame(m) || before[2]!=before[6] || after[2]!=after[6])return false;
 const bool run=m[5]==1;
 unsigned participants=0;for(unsigned role=0;role<3;role++){
  unsigned n=m[PioneerQ38FfnResidualGraphRoleOffset(role)+2];if(!n || n>119)return false;participants+=n;
 }
 const unsigned waits=run?119+2*88+participants:357;
 const unsigned constant=run?119*16+88*47+31*participants+8+92+4:7378;
 const unsigned writes=run?268+2*participants+PioneerQ38FfnResidualOutputPackets(m,0)
  +PioneerQ38FfnResidualOutputPackets(m,1)+PioneerQ38FfnResidualOutputPackets(m,2):238;
 const unsigned writeFlits=run?3351+2*participants:238;
 // QFG1 gather/producer/matrix extra5396flits +4 new reads*(bytes-64)/64.
 // Fixed total output-row flits;each matrix-proof packet adds3 extra flits.
 const unsigned readExtra=run?6032-3*(357-participants):0;
 if(s[10]<waits || uint64_t(s[10])>uint64_t(waits)*65536 || s[11]<waits || s[11]>s[10]
  || (!run && s[11]!=357) || uint64_t(s[6])<uint64_t(s[10])+3*uint64_t(s[11])+constant
  || uint64_t(s[6])>4*uint64_t(s[10])+constant || uint64_t(s[6])+readExtra>UINT32_MAX)return false;
 for(unsigned i=0;i<64;i++){
  uint32_t expected=0;
  if(i==0)expected=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER;
  else if(i==1)expected=3;
  else if(i==2)expected=m[5];
  else if(i==6 || i==10 || i==11)continue;
  else if(i==7)expected=writes;
  else if(i==8)expected=s[6]+readExtra;
  else if(i==9)expected=writeFlits;
  else if(i==12 || i==54)expected=m[13];
  else if(i>=16 && i<24)expected=before[i-16];
  else if(i>=24 && i<32){unsigned c=i-24,delta=c==0 || c==3?s[6]:c==1?s[8]:c==2 || c==6?s[6]+s[7]:c==5?s[9]:s[7];
   expected=before[c]+delta;if(after[c]!=expected)return false;
  }
  else if(i>=32 && i<48)expected=m[16+i-32];
  else if(i==48)expected=unsigned(run);
  else if(i==49)expected=run?9:1;
  else if(i>=50 && i<=53){const unsigned value[]={3,20,68,20480};expected=run?value[i-50]:0;}
  else if(i==55)expected=m[42];
  else if(i==56)expected=1;
  else if(i>=57){const unsigned value[]={0x33524651,20480,4,2,m[13],m[40],m[41]};expected=run?value[i-57]:0;}
  if(s[i]!=expected)return false;
 }
 return true;
}
#endif
