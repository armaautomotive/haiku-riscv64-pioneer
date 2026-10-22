/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_JOBS_H
#define PIONEER_Q38_PROJECTION_JOBS_H
#include "PioneerQ38CanonicalModelPlan.h"
// KERNEL-PRIVATE HEAP immutable GEOMETRY. Same119 persistent peers,all
// learned Q8_0/Q4_K/Q6_K tensor shapes,including3-worker48-row projections
// and9-wave vocabulary matrices. No caller parts/coordinates,source or
// producer claim,IO,code replacement,boot/reset or native launch authority.
struct PioneerQ38ProjectionPart {uint32_t stripe,bank,firstRow,groups;uint64_t offset;};
class PioneerQ38ProjectionJobs {
 const PioneerQ38CanonicalModelPlan* plan=nullptr;
 const PioneerQ38CanonicalPart* stripes[7]={};
 PioneerQ38ProjectionPart parts[1024]={};
 uint32_t selected=65,role=14,type=0,inputs=0,outputs=0,count=0,stripeCount=0;
 uint64_t position=0;
 bool attempted=false,ready=false;
public:
 enum {Workers=119,MaxGroups=16,MaxParts=1024};
 PioneerQ38ProjectionJobs()=default;
 PioneerQ38ProjectionJobs(const PioneerQ38ProjectionJobs&)=delete;
 PioneerQ38ProjectionJobs& operator=(const PioneerQ38ProjectionJobs&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned semanticRole,uint64_t token){
  if(attempted)return false;
  attempted=true;
  if(!canonical.ProjectionGeometry(layer,semanticRole,type,inputs,outputs) || (outputs&15))return false;
  plan=&canonical;selected=layer;role=semanticRole;position=token;stripeCount=outputs/64<7?1:7;
  unsigned groups[7]={},quota[7]={},done[7]={},emitted[7]={},order[7]={0,1,2,3,4,5,6},minimum=0,total=0;
  for(unsigned s=0;s<stripeCount;s++){
   const auto* p=canonical.ProjectionStripe(layer,semanticRole,s);if(!p || !p->rows || (p->rows&15) || p->type!=type)return false;
   stripes[s]=p;groups[s]=p->rows/16;quota[s]=(groups[s]+15)/16;minimum+=quota[s];total+=groups[s];
  }
  const unsigned active=total<Workers?total:unsigned(Workers),target=minimum<active?active:minimum;
  if(total!=outputs/16 || !target || target>MaxParts)return false;
  for(unsigned extra=minimum;extra<target;extra++){
   unsigned best=stripeCount;
   for(unsigned s=0;s<stripeCount;s++)if(quota[s]<groups[s]
    && (best==stripeCount || uint64_t(groups[s])*quota[best]>uint64_t(groups[best])*quota[s]))best=s;
   if(best==stripeCount)return false;
   quota[best]++;
  }
  for(unsigned a=0;a<stripeCount;a++)for(unsigned b=a+1;b<stripeCount;b++)if(stripes[order[b]]->bank<stripes[order[a]]->bank){
   unsigned swap=order[a];order[a]=order[b];order[b]=swap;
  }
  while(count<target){
   bool progress=false;
   for(unsigned ordinal=0;ordinal<stripeCount;ordinal++){
    unsigned s=order[ordinal];if(emitted[s]==quota[s])continue;
    unsigned n=groups[s]/quota[s]+unsigned(emitted[s]<groups[s]%quota[s]);const auto& p=*stripes[s];
    uint64_t offset=uint64_t(p.offset)+uint64_t(done[s])*16*p.rowBytes;
    if(count>=MaxParts || !n || n>16 || done[s]>groups[s] || n>groups[s]-done[s] || (offset&63)
     || uint64_t(n)*16*p.rowBytes>uint64_t(p.bytes)-uint64_t(done[s])*16*p.rowBytes)return false;
    parts[count++]={s,p.bank,p.firstRow+done[s]*16,n,offset};done[s]+=n;emitted[s]++;progress=true;
   }
   if(!progress)return false;
  }
  for(unsigned s=0;s<stripeCount;s++)if(done[s]!=groups[s] || emitted[s]!=quota[s])return false;
  ready=true;if(Matches(canonical,layer,semanticRole,token))return true;
  ready=false;return false;
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& canonical,unsigned layer,unsigned semanticRole,uint64_t token)const {
  unsigned t=0,k=0,n=0;
  if(!ready || plan!=&canonical || selected!=layer || role!=semanticRole || position!=token
   || !canonical.ProjectionGeometry(layer,semanticRole,t,k,n) || t!=type || k!=inputs || n!=outputs
   || !count || count>MaxParts || stripeCount!=(n/64<7?1U:7U))return false;
  unsigned done[7]={},minimum=0,total=0;
  for(unsigned s=0;s<stripeCount;s++){
   if(stripes[s]!=canonical.ProjectionStripe(layer,semanticRole,s))return false;
   unsigned groups=stripes[s]->rows/16;minimum+=(groups+15)/16;total+=groups;
  }
  unsigned active=total<Workers?total:unsigned(Workers);if(count!=(minimum<active?active:minimum))return false;
  for(unsigned i=0;i<count;i++){
   const auto& j=parts[i];if(j.stripe>=stripeCount || !j.groups || j.groups>16)return false;
   const auto& p=*stripes[j.stripe];
   if(j.bank!=p.bank || done[j.stripe]>p.rows/16 || j.groups>p.rows/16-done[j.stripe]
    || j.firstRow!=p.firstRow+done[j.stripe]*16 || j.offset!=uint64_t(p.offset)+uint64_t(done[j.stripe])*16*p.rowBytes)return false;
   done[j.stripe]+=j.groups;
  }
  for(unsigned s=0;s<stripeCount;s++)if(done[s]!=stripes[s]->rows/16)return false;
  return true;
 }
 // Immutable metadata only;callers must bracket use with Matches plus their
 // actual authenticated source/ledger/native owner checks. No public ready
 // flag here can authorize a device operation.
 unsigned Count()const{return ready?count:0;}
 unsigned Waves()const{return ready?(count+Workers-1)/Workers:0;}
 unsigned WaveWorkers(unsigned wave)const{return wave<Waves()?(count-wave*Workers<Workers?count-wave*Workers:unsigned(Workers)):0;}
 const PioneerQ38ProjectionPart* Part(unsigned ordinal)const{return ready && ordinal<count?&parts[ordinal]:nullptr;}
 unsigned Type()const{return ready?type:0;}
 unsigned Inputs()const{return ready?inputs:0;}
 unsigned Outputs()const{return ready?outputs:0;}
 bool LearnedPagesPresent(const uint8_t* initialized,unsigned pages)const {
  return plan && Matches(*plan,selected,role,position) && plan->ProjectionPagesPresent(selected,role,initialized,pages);
 }
};
static_assert(sizeof(PioneerQ38ProjectionJobs)>24576 && sizeof(PioneerQ38ProjectionJobs)<28672,"multi-wave projection geometry HEAP ONLY");
#endif
