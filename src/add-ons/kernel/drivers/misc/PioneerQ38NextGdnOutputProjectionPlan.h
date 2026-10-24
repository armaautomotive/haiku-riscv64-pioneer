/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_GDN_OUTPUT_PROJECTION_PLAN_H
#define PIONEER_Q38_NEXT_GDN_OUTPUT_PROJECTION_PLAN_H
#if !defined(PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE) || PIONEER_Q38_NEXT_GDN_OUTPUT_NATIVE != 1 || !defined(PIONEER_Q38_NEXT_GDN_NATIVE) || PIONEER_Q38_NEXT_GDN_NATIVE != 1
#error Exact SAME native62 consumer of genuine native61 required
#endif
#include "PioneerQ38ProjectionJobs.h"
#include "PioneerQ38DispatchBootstrap.h"
#include "PioneerQ38NextGdnOutputLearned.h"
#include "PioneerQ38NextGdnProducer.h"
#include "PioneerQ38NextGdnOutputInputShape.h"
#include "PioneerQ38NextGdnOutputGatherShape.h"
// PRIVATE HEAP DATA for the NEXT distinct ssm_out input/image family.
// Does not widen old QPS1/RMS input authority or execute a projection.
// No caller token/layer/addresses/epoch/activation or publication hints.
// A native consumer must retain this exact producer and all its parents,
// recheck its real IO scope/physical state,and use a separately pinned
// root/peer/gather/output image. The old RMS graph cannot consume this.
class PioneerQ38NextGdnOutputProjectionPlan {
 friend class PioneerQ38NextGdnOutputBootstrapPlan;
 friend class PioneerQ38ResidentDecoderOwner;
 const PioneerQ38NextGdnProducer* producer=nullptr;
 PioneerQ38ProjectionJobs jobs;
 bool attempted=false,ready=false;
public:
 enum {InputWords=6144,OutputWords=5120,OutputOffset=TT_Q38_GDN_GATED_OUTPUT+InputWords*4};
 PioneerQ38NextGdnOutputProjectionPlan()=default;
 PioneerQ38NextGdnOutputProjectionPlan(const PioneerQ38NextGdnOutputProjectionPlan&)=delete;
 PioneerQ38NextGdnOutputProjectionPlan& operator=(const PioneerQ38NextGdnOutputProjectionPlan&)=delete;
 bool Build(const PioneerQ38NextGdnProducer& input){
  if(attempted)return false;
  attempted=true;
  if(!input.Current() || !input.canonical || !input.ledger || input.position || input.layer!=1 || input.bank>=7 || input.phase>=0x7ffffffd
   || !jobs.Build(*input.canonical,1,13,input.position) || jobs.Type()!=8 || jobs.Inputs()!=InputWords || jobs.Outputs()!=OutputWords
   || jobs.Count()!=119 || jobs.Waves()!=1 || !jobs.LearnedPagesPresent(input.ledger,input.pages) || !PioneerQ38NextGdnOutputLearned(*input.canonical,input.ledger,input.pages))return false;
  producer=&input;ready=true;
  if(Current())return true;
  ready=false;return false;
 }
 bool Current()const{
  if(!ready || !producer || !producer->Current() || !producer->canonical || !producer->ledger
   || !jobs.Matches(*producer->canonical,1,13,producer->position) || !jobs.LearnedPagesPresent(producer->ledger,producer->pages) || producer->layer!=1 || producer->position || !PioneerQ38NextGdnOutputLearned(*producer->canonical,producer->ledger,producer->pages))return false;
  const auto* workspace=producer->canonical->Workspace(producer->bank);
  return workspace && workspace->bytes==67108864 && workspace->allocated==67108864
   && producer->inputAddress==producer->canonical->Base+workspace->offset+TT_Q38_GDN_GATED_OUTPUT;
 }
 const PioneerQ38NextGdnProducer* Producer()const{return Current()?producer:nullptr;}
 const PioneerQ38ProjectionPart* Part(unsigned ordinal)const{return Current()?jobs.Part(ordinal):nullptr;}
 unsigned Count()const{return Current()?jobs.Count():0;}
 uint32_t InputAddress()const{return Current()?producer->inputAddress:0;}
 uint32_t OutputAddress()const{return Current()?producer->inputAddress-TT_Q38_GDN_GATED_OUTPUT+OutputOffset:0;}
 unsigned Bank()const{return Current()?producer->bank:7;}
 unsigned Phase()const{return Current()?producer->phase+1:0;}
 bool InputDescriptor(unsigned chunk,const PioneerQ38DispatchLease& cohort,uint32_t (&out)[64])const{
  if(!Current() || chunk>=24 || !producer->MatchesCohort(cohort))return false;
  uint32_t d[64]={};
  d[0]=TT_Q38_NEXT_GDN_OUTPUT_INPUT_MARKER;d[1]=1;d[2]=67108864;d[3]=producer->bank;
  d[4]=producer->inputAddress-TT_Q38_GDN_GATED_OUTPUT;d[5]=chunk;d[6]=producer->phase+1;d[7]=chunk+1;
  d[8]=PioneerQ38ProjectionRoute(d[3]);d[9]=PioneerQ38ProjectionPeer(chunk);d[10]=TT_Q38_GDN_GATED_OUTPUT+chunk*1024;
  d[11]=6144;d[12]=TT_Q38_PROJECTION_NATIVE_OFFSET+chunk*512;d[13]=TT_Q38_PROJECTION_LINES_OFFSET+chunk*128;d[14]=8;d[15]=24;
  for(unsigned i=0;i<16;i++)d[16+i]=cohort.authority.source[chunk+1][i];
  const uint64_t ids[]={producer->identity.session,producer->identity.sequence,producer->identity.modelEpoch,producer->identity.driverGeneration};
  for(unsigned i=0;i<4;i++){d[32+i*2]=uint32_t(ids[i]);d[33+i*2]=uint32_t(ids[i]>>32);}
  d[40]=producer->position;d[41]=producer->phase;d[42]=producer->layer;d[43]=13;d[44]=TT_Q38_GDN_RENDEZVOUS_MARKER;d[45]=producer->token;
  if(!PioneerQ38NextGdnOutputInputShape(d) || !Current() || !producer->MatchesCohort(cohort))return false;
  for(unsigned i=0;i<64;i++)out[i]=d[i];
  return true; // CPU DATA only,never an upload or launch
 }
 bool MatchesSource(const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38GdnIdentity& id,
  const uint8_t* ledger,unsigned pages,const uint32_t* rootSource)const{
  if(!Current() || &canonical!=producer->canonical || !(id==producer->identity) || ledger!=producer->ledger || pages!=producer->pages || !rootSource)return false;
  for(unsigned i=0;i<16;i++)if(rootSource[i]!=producer->source[i])return false;
  return true;
 }
 bool GatherDescriptor(const PioneerQ38DispatchLease& cohort,uint32_t (&out)[64])const{
  uint32_t d[64];if(!InputDescriptor(0,cohort,d))return false;
  d[0]=TT_Q38_NEXT_GDN_OUTPUT_GATHER_MARKER;d[5]=24;d[7]=0;d[9]=129;
  d[10]=TT_Q38_PROJECTION_NATIVE_OFFSET;d[11]=TT_Q38_PROJECTION_LINES_OFFSET;d[12]=TT_Q38_PROJECTION_COMPACT_OFFSET;
  d[13]=12288;d[14]=768;d[15]=8;d[46]=TT_Q38_NEXT_GDN_OUTPUT_GATHER_SOURCES;d[47]=24;
  for(unsigned i=0;i<16;i++)d[16+i]=cohort.authority.source[0][i];
  if(!PioneerQ38NextGdnOutputGatherShape(d) || !Current() || !producer->MatchesCohort(cohort))return false;
  for(unsigned i=0;i<64;i++)out[i]=d[i];
  return true; // CPU metadata only;does not authorize ROOT entry or NoC
 }
 bool GatherSources(const PioneerQ38DispatchLease& cohort,uint32_t (&out)[24][16])const{
  if(!Current() || !producer->MatchesCohort(cohort))return false;
  uint32_t staged[24][16];
  for(unsigned w=0;w<24;w++){
   if(!cohort.authority.source[w+1][0])return false;
   for(unsigned i=0;i<16;i++)staged[w][i]=cohort.authority.source[w+1][i];
  }
  if(!Current() || !producer->MatchesCohort(cohort))return false;
  for(unsigned w=0;w<24;w++)for(unsigned i=0;i<16;i++)out[w][i]=staged[w][i];
  return true; // NO caller source hints/addresses or activation copy
 }
};
static_assert(PioneerQ38NextGdnOutputProjectionPlan::OutputOffset+PioneerQ38NextGdnOutputProjectionPlan::OutputWords*4<=2097152,"gated input and ssm_out output disjoint inside retained initialized prefix");
static_assert(sizeof(PioneerQ38NextGdnOutputProjectionPlan)>24576 && sizeof(PioneerQ38NextGdnOutputProjectionPlan)<32768,"canonical complete output jobs HEAP ONLY");
#endif
