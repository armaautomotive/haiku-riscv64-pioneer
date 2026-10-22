/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_MATRIX_PROOF_H
#define PIONEER_Q38_FFN_MATRIX_PROOF_H
#include <stddef.h>
#include "PioneerQ38FfnMatrixShape.h"
#include "PioneerQ38CohortProtocol.h"
#include "PioneerQ38RootCycleData.h"
// KERNEL-PRIVATE logical snapshot ONLY. No host inputs,scales,outputs or
// physical addresses;never expose as an ioctl or infer producer ownership.
struct PioneerQ38FfnMatrixJob {
 uint32_t role=0,layer=0,type=0,peers=0;
 uint64_t position=0;
 tt_q38_cohort_part part[119]={};
 uint32_t first[119]={};
};
// Large HEAP-only staging/expected authority. Generated ONLY after complete
// initialized learned-range and reserved-workspace authorization. Data here
// is not a grant to start/hold/restore/read a model or release any owner.
struct PioneerQ38FfnMatrixAuthority {
 uint32_t header[16]={},frame[64]={},banks[8]={},task[119][8]={},first[119]={};
 PioneerQ38CohortExtent extent[119];
};
static inline bool PioneerQ38FfnMatrixJobValid(const PioneerQ38FfnMatrixJob& j)
{
 if(j.role>2 || j.layer>=64 || (j.type!=12 && j.type!=14) || !j.peers || j.peers>119)return false;
 unsigned rows=0,outputs=j.role==2?5120:17408,inputs=j.role==2?17408:5120;
 for(unsigned w=0;w<119;w++) {
  const auto& p=j.part[w];
  if(w>=j.peers){if(p.bank || p.groups || p.offset || j.first[w])return false;continue;}
  if(!PioneerQ38CohortPartValid(j.type,inputs,p) || (j.first[w]&15) || j.first[w]>=outputs || p.groups*16>outputs-j.first[w])return false;
  for(unsigned before=0;before<w;before++) {
   const auto& q=j.part[before];
   if((j.first[w]<j.first[before]+q.groups*16 && j.first[before]<j.first[w]+p.groups*16)
    || (p.bank==q.bank && p.offset<q.offset+PioneerQ38CohortBytes(j.type,inputs,q.groups)
    && q.offset<p.offset+PioneerQ38CohortBytes(j.type,inputs,p.groups)))return false;
  }
  rows+=p.groups*16;
 }
 return rows==outputs;
}
static inline bool PioneerQ38FfnMatrixProof(const PioneerQ38FfnMatrixAuthority& a,const uint32_t* before,
 const uint32_t* after,const uint32_t* stats)
{
 const auto* h=a.header;const auto* d=a.frame;const auto* s=stats;
 if(!before || !after || !s || !PioneerQ38FfnMatrixShape(d,h,a.banks,a.task,a.first)
  || before[2]!=before[6] || after[2]!=after[6] || s[0]!=TT_Q38_FFN_MATRIX_MARKER || s[1]!=1 || s[2]!=1
  || s[3]!=h[3] || s[4]!=h[4] || s[5]!=h[5] || s[12]!=h[6] || s[13] || s[14] || s[15]
  || s[10]<h[5] || s[11]<h[5] || s[11]>s[10] || s[10]>65536*h[5] || s[48]!=1
  || s[49]!=TT_Q38_FFN_MATRIX_MARKER || s[50]!=PioneerQ38FfnMatrixInputReads(d[5])
  || s[51]!=PioneerQ38FfnMatrixInputFlits(d[5]) || s[53]!=d[12]/64 || s[54]!=d[5] || s[55]!=d[11] || s[56]!=1
  || !PioneerQ38RootCycleDataValid(s))return false;
 const unsigned outputPackets=PioneerQ38FfnMatrixOrdered(a.task,a.first,h[5])?(d[12]+16383)/16384:h[5];
 const uint32_t groups=d[11]/16,reads=s[10]+3*s[11]+h[5]+s[50],flits=s[10]+3*s[11]+4*h[5]+groups+s[51];
 const uint32_t writes=2*h[5]+outputPackets,writeFlits=2*h[5]+groups;
 if(s[52]!=outputPackets || s[6]!=reads || s[7]!=writes || s[8]!=flits || s[9]!=writeFlits)return false;
 for(unsigned i=0;i<8;i++) {
  unsigned delta=i==0 || i==3?reads:i==1?flits:i==2 || i==6?reads+writes:i==5?writeFlits:writes;
  if(s[16+i]!=before[i] || s[24+i]!=after[i] || uint32_t(after[i]-before[i])!=delta)return false;
 }
 for(unsigned i=0;i<16;i++)if(s[32+i]!=d[16+i])return false;
 return true; // DATA relation ONLY;controller still owes physical/lifetime proofs
}
static inline bool PioneerQ38FfnMatrixJobMatches(const PioneerQ38FfnMatrixJob& j,const PioneerQ38FfnMatrixAuthority& a)
{
 if(!PioneerQ38FfnMatrixJobValid(j) || a.header[3]!=j.type || a.header[5]!=j.peers
  || a.frame[5]!=j.role || a.frame[42]!=j.layer || a.frame[40]!=uint32_t(j.position) || a.frame[41]!=uint32_t(j.position>>32))return false;
 for(unsigned w=0;w<119;w++) {
  if(a.first[w]!=j.first[w])return false;
  if(w<j.peers && (a.task[w][0]!=PioneerResidentBanks::Base+j.part[w].offset
   || a.task[w][1]!=j.part[w].bank || a.task[w][2]!=j.part[w].groups))return false;
 }
 return true;
}
static inline bool PioneerQ38FfnMatrixLastCommand(const PioneerQ38FfnMatrixAuthority& a,uint32_t (&command)[10])
{
 if(!PioneerQ38FfnMatrixShape(a.frame,a.header,a.banks,a.task,a.first))return false;
 const auto* d=a.frame;unsigned local=0,remote=0,bytes=0;
 if(PioneerQ38FfnMatrixOrdered(a.task,a.first,d[7])) {
  unsigned at=(d[12]-1)&~16383U;local=0x80000+at;remote=d[4]+d[10]+at;bytes=d[12]-at;
 }else {
  unsigned packed=0;for(unsigned w=0;w+1<d[7];w++)packed+=a.task[w][2]*64;
  local=0x80000+packed;remote=d[4]+d[10]+a.first[d[7]-1]*4;bytes=a.task[d[7]-1][2]*64;
 }
 const uint32_t value[]={local,0,129,remote,0,d[8],0,0x212,bytes,0};for(unsigned i=0;i<10;i++)command[i]=value[i];
 return true;
}
#endif
