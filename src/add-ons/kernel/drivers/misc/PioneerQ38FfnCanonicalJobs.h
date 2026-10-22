/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_CANONICAL_JOBS_H
#define PIONEER_Q38_FFN_CANONICAL_JOBS_H
#include "PioneerQ38CanonicalModelPlan.h"
#include "PioneerQ38FfnMatrixProof.h"
// KERNEL-PRIVATE,HEAP ONLY. Derive all THREE complete119-peer FFN jobs
// from the fixed model layout,not caller-provided offsets/rows/banks. This
// borrows the exact immutable plan;it outlives these proposals. Metadata
// is NOT source authentication,an initialized input receipt,code/clock/
// host/current-terminal ownership,or permission to launch any operation.
class PioneerQ38FfnCanonicalJobs {
 const PioneerQ38CanonicalModelPlan* plan=nullptr;
 PioneerQ38FfnMatrixJob jobs[3];
 unsigned layer=64;
 uint64_t position=0;
 bool attempted=false,ready=false;
 bool Role(unsigned role) {
  unsigned groups[7]={},quota[7]={},emitted[7]={},done[7]={},order[7]={0,1,2,3,4,5,6},minimum=0;
  for(unsigned stripe=0;stripe<7;stripe++) {
   const auto& p=*plan->FfnStripe(layer,role,stripe);
   if((p.rows&15) || !p.rows || p.type!=12)return false;
   groups[stripe]=p.rows/16;quota[stripe]=(groups[stripe]+15)/16;minimum+=quota[stripe];
  }
  if(minimum>119)return false;
  for(unsigned extra=minimum;extra<119;extra++) {
   unsigned best=7;
   for(unsigned stripe=0;stripe<7;stripe++)if(quota[stripe]<groups[stripe]
    && (best==7 || uint64_t(groups[stripe])*quota[best]>uint64_t(groups[best])*quota[stripe]))best=stripe;
   if(best==7)return false;
   quota[best]++;
  }
  // Preserve the existing loader's deterministic bank-order round robin,
  // including the semantic-first-row tie breaker during quota balancing.
  for(unsigned a=0;a<7;a++)for(unsigned b=a+1;b<7;b++)
   if(plan->FfnStripe(layer,role,order[b])->bank<plan->FfnStripe(layer,role,order[a])->bank){
    unsigned swap=order[a];order[a]=order[b];order[b]=swap;}
  auto& j=jobs[role];j.role=role;j.layer=layer;j.type=12;j.peers=119;j.position=position;
  unsigned worker=0;
  while(worker<119) {
   bool progress=false;
   for(unsigned ordinal=0;ordinal<7;ordinal++) {
    unsigned stripe=order[ordinal];if(emitted[stripe]==quota[stripe])continue;
    const auto& p=*plan->FfnStripe(layer,role,stripe);
    unsigned n=groups[stripe]/quota[stripe]+unsigned(emitted[stripe]<groups[stripe]%quota[stripe]);
    uint64_t offset=uint64_t(p.offset)+uint64_t(done[stripe])*16*p.rowBytes;
    if(worker>=119 || !n || n>16 || !plan->FfnSpan(layer,role,p.bank,offset,p.firstRow+done[stripe]*16,n*16))return false;
    j.part[worker]={p.bank,n,offset};j.first[worker]=p.firstRow+done[stripe]*16;
    done[stripe]+=n;emitted[stripe]++;worker++;progress=true;
   }
   if(!progress)return false;
  }
  for(unsigned stripe=0;stripe<7;stripe++)if(done[stripe]!=groups[stripe] || emitted[stripe]!=quota[stripe])return false;
  return PioneerQ38FfnMatrixJobValid(j);
 }
public:
 PioneerQ38FfnCanonicalJobs()=default;
 PioneerQ38FfnCanonicalJobs(const PioneerQ38FfnCanonicalJobs&)=delete;
 PioneerQ38FfnCanonicalJobs& operator=(const PioneerQ38FfnCanonicalJobs&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,unsigned selected,uint64_t token) {
  if(attempted)return false;
  attempted=true;
  if(!canonical.Ready() || selected>=64)return false;
  plan=&canonical;layer=selected;position=token;
  for(unsigned role=0;role<3;role++)if(!Role(role))return false;
  ready=true;return true;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,unsigned selected,uint64_t token)const {
  if(!ready || plan!=&canonical || !canonical.Ready() || layer!=selected || position!=token)return false;
  for(unsigned role=0;role<3;role++) {
   const auto& j=jobs[role];
   if(j.layer!=layer || j.role!=role || j.type!=12 || j.peers!=119 || j.position!=position || !PioneerQ38FfnMatrixJobValid(j))return false;
   for(unsigned w=0;w<119;w++)if(!canonical.FfnSpan(layer,role,j.part[w].bank,j.part[w].offset,j.first[w],j.part[w].groups*16))return false;
  }
  return true;
 }
 const PioneerQ38FfnMatrixJob* Job(unsigned role)const{return plan && role<3 && Matches(*plan,layer,position)?&jobs[role]:nullptr;}
 bool LearnedPagesPresent(const uint8_t* initialized,unsigned pages)const {
  return plan && Matches(*plan,layer,position) && plan->FfnPagesPresent(layer,initialized,pages);
 }
};
static_assert(sizeof(PioneerQ38FfnCanonicalJobs)>6144 && sizeof(PioneerQ38FfnCanonicalJobs)<8192,"ALL3 complete FFN jobs must remain HEAP ONLY");
#endif
