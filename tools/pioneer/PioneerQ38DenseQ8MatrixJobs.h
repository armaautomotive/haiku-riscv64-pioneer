/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DENSE_Q8_MATRIX_JOBS_H
#define PIONEER_Q38_DENSE_Q8_MATRIX_JOBS_H
#if !defined(PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION) || PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Dense Q8 matrix jobs are offline geometry only
#endif
#include "PioneerQ38HybridDenseQ8MatrixPlan.h"
#include "PioneerQ38Bfp8WorkerJob.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38CanonicalModelPlan.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"

// Separate 32-row job plan. Original Q8 jobs use 16-row units and cannot be
// reinterpreted as BFP8 jobs. This only constructs DATA, never dispatches.
class PioneerQ38DenseQ8MatrixJobs {
 using Canonical=PioneerQ38CanonicalModelPlan;using Plan=PioneerQ38HybridDenseQ8MatrixPlan;using W=PioneerQ38Bfp8WorkerJob;
 struct Part {uint32_t bank,first,bands,address;};
 Part parts[119]={};const Canonical* source=nullptr;unsigned selected=64,role=0,count=0,outputRows=0;
 uint64_t position=0;bool attempted=false,ready=false;
public:
 PioneerQ38DenseQ8MatrixJobs()=default;
 PioneerQ38DenseQ8MatrixJobs(const PioneerQ38DenseQ8MatrixJobs&)=delete;
 PioneerQ38DenseQ8MatrixJobs& operator=(const PioneerQ38DenseQ8MatrixJobs&)=delete;
 bool Build(const Canonical& canonical,unsigned layer,unsigned semanticRole,uint64_t token){
  if(attempted)return false;attempted=true;
  unsigned type=0,k=0,n=0;
  if(layer>=64 || layer%4!=3 || semanticRole<5 || semanticRole>7 || token>=Canonical::Context
   || !canonical.ProjectionGeometry(layer,semanticRole,type,k,n) || type!=8 || k!=5120 || (n!=12288 && n!=1024))return false;
  Plan::Proposal plan;if(!Plan::Build(canonical,plan))return false;
  const PioneerQ38CanonicalPart* stripe[7]={};unsigned index[7]={},units[7]={},quota[7]={},done[7]={},emitted[7]={},order[7]={0,1,2,3,4,5,6};
  unsigned minimum=0,total=0;
  for(unsigned s=0;s<7;s++){
   stripe[s]=canonical.ProjectionStripe(layer,semanticRole,s);if(!stripe[s] || !stripe[s]->rows || (stripe[s]->rows&31))return false;
   index[s]=canonical.Count();for(unsigned i=0;i<canonical.Count();i++)if(canonical.Part(i)==stripe[s]){index[s]=i;break;}
   if(index[s]>=plan.parts.size() || plan.parts[index[s]].encoding!=Plan::DenseQ8Bfp8)return false;
   units[s]=stripe[s]->rows/32;quota[s]=(units[s]+7)/8;minimum+=quota[s];total+=units[s];
  }
  const unsigned active=total<119?total:119,target=minimum<active?active:minimum;
  if(total!=n/32 || !target || target>119)return false;
  for(unsigned extra=minimum;extra<target;extra++){
   unsigned best=7;
   for(unsigned s=0;s<7;s++)if(quota[s]<units[s]
    && (best==7 || uint64_t(units[s])*quota[best]>uint64_t(units[best])*quota[s]))best=s;
   if(best==7)return false;quota[best]++;
  }
  for(unsigned a=0;a<7;a++)for(unsigned b=a+1;b<7;b++)if(stripe[order[b]]->bank<stripe[order[a]]->bank){unsigned x=order[a];order[a]=order[b];order[b]=x;}
  Part staged[119]={};unsigned emittedTotal=0;
  while(emittedTotal<target){
   bool progress=false;
   for(unsigned o=0;o<7;o++){
    unsigned s=order[o];if(emitted[s]==quota[s])continue;
    const unsigned bands=units[s]/quota[s]+unsigned(emitted[s]<units[s]%quota[s]);const auto& relocated=plan.parts[index[s]];
    const uint64_t within=uint64_t(done[s])*174080,bytes=uint64_t(bands)*174080;
    if(!bands || bands>8 || done[s]>units[s] || bands>units[s]-done[s]
     || relocated.bank!=stripe[s]->bank || within>relocated.bytes || bytes>uint64_t(relocated.bytes)-within
     || uint64_t(relocated.offset)+within>UINT32_MAX-Canonical::Base)return false;
    staged[emittedTotal++]={stripe[s]->bank,stripe[s]->firstRow+done[s]*32,bands,uint32_t(Canonical::Base+relocated.offset+within)};
    done[s]+=bands;emitted[s]++;progress=true;
   }
   if(!progress)return false;
  }
  for(unsigned s=0;s<7;s++)if(done[s]!=units[s] || emitted[s]!=quota[s])return false;
  for(unsigned i=0;i<target;i++)parts[i]=staged[i];
  source=&canonical;selected=layer;role=semanticRole;position=token;count=target;outputRows=n;ready=true;return true;
 }
 unsigned Count()const{return ready?count:0;}
 unsigned Rows()const{return ready?outputRows:0;}
 bool Job(const Canonical& canonical,unsigned layer,unsigned semanticRole,uint64_t token,unsigned ordinal,unsigned sequence,
  W::Job& output,unsigned& firstRow)const{
  if(!ready || source!=&canonical || !canonical.Ready() || layer!=selected || semanticRole!=role || token!=position
   || ordinal>=count || !sequence || sequence>=0x7fffffff)return false;
  const uintptr_t a=reinterpret_cast<uintptr_t>(&output),b=reinterpret_cast<uintptr_t>(&firstRow);
  const uintptr_t self=reinterpret_cast<uintptr_t>(this),model=reinterpret_cast<uintptr_t>(&canonical);
  if(a>UINTPTR_MAX-sizeof(output) || b>UINTPTR_MAX-sizeof(firstRow) || self>UINTPTR_MAX-sizeof(*this)
   || model>UINTPTR_MAX-sizeof(canonical) || (a<b+sizeof(firstRow) && b<a+sizeof(output))
   || (a<self+sizeof(*this) && self<a+sizeof(output)) || (b<self+sizeof(*this) && self<b+sizeof(firstRow))
   || (a<model+sizeof(canonical) && model<a+sizeof(output)) || (b<model+sizeof(canonical) && model<b+sizeof(firstRow)))return false;
  const auto& p=parts[ordinal];const unsigned worker=ordinal+1;
  W::Job next={{W::Magic,W::Version,5120,p.address,p.bands,PioneerQ38ProjectionRoute(p.bank),worker,W::Existing::Peer(worker),
   PioneerQ38Bf16Wire::Magic,W::RootInput,W::RootRoute,sequence,W::Output,0,0,0}};
  if(!W::Shape(next) || p.first>=outputRows || p.bands*32>outputRows-p.first)return false;
  output=next;firstRow=p.first;return true;
 }
};
#endif
