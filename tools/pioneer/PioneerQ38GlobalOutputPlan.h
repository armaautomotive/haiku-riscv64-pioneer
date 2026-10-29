/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GLOBAL_OUTPUT_PLAN_H
#define PIONEER_Q38_GLOBAL_OUTPUT_PLAN_H
#include "PioneerQ38GlobalWorkerLayout.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionJobs.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnIdentity.h"
// Canonical DATA ONLY. No authenticated weights/ledger, completed all64,
// normalized input producer, native owner, sampling or execution grant.
class PioneerQ38GlobalOutputPlan {
 const PioneerQ38CanonicalModelPlan* canonical=nullptr;
 PioneerQ38ProjectionJobs jobs;uint32_t sources[1920]={},identity[8]={};
 unsigned position=8192,normBank=7,normAddress=0;bool attempted=false,ready=false;
 bool Norm(const PioneerQ38CanonicalModelPlan& c,unsigned& bank,unsigned& address)const{
  bool found=false;
  for(unsigned i=0;i<c.Count();i++){
   const auto* p=c.Part(i);if(p->kind!=c.WeightKind || p->layer!=64 || p->role!=2)continue;
   if(found || p->type || p->bank>=7 || p->firstRow || p->rows!=1 || p->rowBytes!=20480 || p->bytes!=20480 || p->allocated!=20480)return false;
   found=true;bank=p->bank;address=c.Base+p->offset;
  }
  return found;
 }
 bool Output(const void* output,unsigned bytes)const{
  uintptr_t o=reinterpret_cast<uintptr_t>(output),self=reinterpret_cast<uintptr_t>(this),c=reinterpret_cast<uintptr_t>(canonical);
  return output && !(o&3) && o<=UINTPTR_MAX-bytes && !(o<self+sizeof(*this) && self<o+bytes)
   && (!canonical || !(o<c+sizeof(*canonical) && c<o+bytes));
 }
public:
 PioneerQ38GlobalOutputPlan()=default;
 PioneerQ38GlobalOutputPlan(const PioneerQ38GlobalOutputPlan&)=delete;
 PioneerQ38GlobalOutputPlan& operator=(const PioneerQ38GlobalOutputPlan&)=delete;
 bool Build(const PioneerQ38CanonicalModelPlan& c,unsigned at,const PioneerQ38GdnIdentity& id,const uint32_t* physicalSources){
  if(attempted)return false;
  attempted=true;
  if(!c.Ready() || at>=8192 || !id.Valid() || !physicalSources || !jobs.Build(c,64,1,at)
   || jobs.Count()!=973 || jobs.Waves()!=9 || jobs.WaveWorkers(8)!=21 || jobs.Type()!=14 || jobs.Inputs()!=5120 || jobs.Outputs()!=248320
   || !Norm(c,normBank,normAddress))return false;
  for(unsigned w=0;w<120;w++)if(!physicalSources[w*16])return false;
  for(unsigned i=0;i<1920;i++)sources[i]=physicalSources[i];
  const uint64_t values[]={id.session,id.sequence,id.modelEpoch,id.driverGeneration};
  for(unsigned i=0;i<4;i++){identity[i*2]=uint32_t(values[i]);identity[i*2+1]=uint32_t(values[i]>>32);}
  canonical=&c;position=at;ready=true;return Matches(c);
 }
 bool Matches(const PioneerQ38CanonicalModelPlan& c)const{
  unsigned bank=7,address=0;
  return ready && canonical==&c && position<8192 && jobs.Matches(c,64,1,position) && Norm(c,bank,address)
   && bank==normBank && address==normAddress;
 }
 unsigned NormBank()const{return ready?normBank:7;}
 unsigned NormAddress()const{return ready?normAddress:0;}
 bool WorkerBoot(unsigned worker,uint32_t* output)const{
  namespace G=PioneerQ38GlobalWorkerLayout;
  if(!canonical || !Matches(*canonical) || !worker || worker>119 || !Output(output,256))return false;
  uint32_t b[64]={};const unsigned h[]={G::Magic,1,64,worker,G::Peer(worker),sources[worker*16],8192,64};
  for(unsigned i=0;i<8;i++){b[i]=h[i];b[8+i]=identity[i];}
  for(unsigned i=0;i<16;i++)b[16+i]=sources[worker*16+i];
  for(unsigned i=40;i<46;i++)b[i]=G::BootWord(i);
  if(!G::BootShape(b))return false;
  for(unsigned i=0;i<64;i++)output[i]=b[i];
  return true;
 }
 bool Job(unsigned wave,unsigned worker,unsigned previous,uint32_t* output)const{
  namespace G=PioneerQ38GlobalWorkerLayout;
  if(!canonical || !Matches(*canonical) || !worker || worker>jobs.WaveWorkers(wave) || previous>=0x7ffffffe || !Output(output,256))return false;
  const auto* p=jobs.Part(wave*119+worker-1);if(!p)return false;
  uint32_t j[64]={},b[64];if(!WorkerBoot(worker,b))return false;
  const unsigned h[]={G::Magic,previous+1,G::GlobalMatrix,worker,G::Peer(worker),64,position,2025+position*4096+wave};
  for(unsigned i=0;i<8;i++){j[i]=h[i];j[8+i]=identity[i];}
  const unsigned m[]={0x31483851,14,3392,0,0,5120,canonical->Base+uint32_t(p->offset),p->groups,PioneerQ38ProjectionRoute(p->bank),1};
  for(unsigned i=0;i<10;i++)j[16+i]=m[i];
  j[26]=1;j[27]=wave;j[28]=p->firstRow;j[29]=248320;j[30]=1;j[31]=G::JobWitness;
  if(!G::JobShape(j,b,previous,0))return false;
  for(unsigned i=0;i<64;i++)output[i]=j[i];
  return true;
 }
};
static_assert(sizeof(PioneerQ38GlobalOutputPlan)>28672 && sizeof(PioneerQ38GlobalOutputPlan)<40000,"global DATA plan HEAP ONLY");
#endif
