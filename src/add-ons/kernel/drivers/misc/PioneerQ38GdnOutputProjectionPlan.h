/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_OUTPUT_PROJECTION_PLAN_H
#define PIONEER_Q38_GDN_OUTPUT_PROJECTION_PLAN_H
#include "PioneerQ38ResidentDecoderOwner.h"
#include "PioneerQ38GdnOutputProducer.h"
// PRIVATE HEAP DATA for the NEXT distinct ssm_out input/image family.
// Does not widen old QPS1/RMS input authority or execute a projection.
// No caller token/layer/addresses/epoch/activation or publication hints.
// A native consumer must retain this exact producer and all its parents,
// recheck its real IO scope/physical state,and use a separately pinned
// root/peer/gather/output image. The old RMS graph cannot consume this.
class PioneerQ38GdnOutputProjectionPlan {
 const PioneerQ38GdnOutputProducer* producer=nullptr;
 PioneerQ38ProjectionJobs jobs;
 bool attempted=false,ready=false;
public:
 enum {InputWords=6144,OutputWords=5120,OutputOffset=TT_Q38_GDN_GATED_OUTPUT+InputWords*4};
 PioneerQ38GdnOutputProjectionPlan()=default;
 PioneerQ38GdnOutputProjectionPlan(const PioneerQ38GdnOutputProjectionPlan&)=delete;
 PioneerQ38GdnOutputProjectionPlan& operator=(const PioneerQ38GdnOutputProjectionPlan&)=delete;
 bool Build(const PioneerQ38GdnOutputProducer& input){
  if(attempted)return false;
  attempted=true;
  if(!input.Current() || !input.canonical || !input.ledger || input.position || input.bank>=7 || input.phase>=0x7ffffffd
   || !jobs.Build(*input.canonical,0,13,input.position) || jobs.Type()!=8 || jobs.Inputs()!=InputWords || jobs.Outputs()!=OutputWords
   || jobs.Count()!=119 || jobs.Waves()!=1 || !jobs.LearnedPagesPresent(input.ledger,input.pages))return false;
  producer=&input;ready=true;
  if(Current())return true;
  ready=false;return false;
 }
 bool Current()const{
  if(!ready || !producer || !producer->Current() || !producer->canonical || !producer->ledger
   || !jobs.Matches(*producer->canonical,0,13,producer->position) || !jobs.LearnedPagesPresent(producer->ledger,producer->pages))return false;
  const auto* workspace=producer->canonical->Workspace(producer->bank);
  return workspace && workspace->bytes==67108864 && workspace->allocated==67108864
   && producer->inputAddress==producer->canonical->Base+workspace->offset+TT_Q38_GDN_GATED_OUTPUT;
 }
 const PioneerQ38GdnOutputProducer* Producer()const{return Current()?producer:nullptr;}
 const PioneerQ38ProjectionPart* Part(unsigned ordinal)const{return Current()?jobs.Part(ordinal):nullptr;}
 unsigned Count()const{return Current()?jobs.Count():0;}
 uint32_t InputAddress()const{return Current()?producer->inputAddress:0;}
 uint32_t OutputAddress()const{return Current()?producer->inputAddress-TT_Q38_GDN_GATED_OUTPUT+OutputOffset:0;}
 unsigned Bank()const{return Current()?producer->bank:7;}
 unsigned Phase()const{return Current()?producer->phase+1:0;}
 bool MatchesSource(const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38GdnIdentity& id,
  const uint8_t* ledger,unsigned pages,const uint32_t* rootSource)const{
  if(!Current() || &canonical!=producer->canonical || !(id==producer->identity) || ledger!=producer->ledger || pages!=producer->pages || !rootSource)return false;
  for(unsigned i=0;i<16;i++)if(rootSource[i]!=producer->source[i])return false;
  return true;
 }
};
static_assert(PioneerQ38GdnOutputProjectionPlan::OutputOffset+PioneerQ38GdnOutputProjectionPlan::OutputWords*4<=2097152,"gated input and ssm_out output disjoint inside retained initialized prefix");
static_assert(sizeof(PioneerQ38GdnOutputProjectionPlan)>24576 && sizeof(PioneerQ38GdnOutputProjectionPlan)<32768,"canonical complete output jobs HEAP ONLY");
#endif
