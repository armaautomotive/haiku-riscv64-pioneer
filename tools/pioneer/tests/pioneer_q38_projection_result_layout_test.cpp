/* SPDX-License-Identifier: MIT */
// Fixed buffer/manifest DATA only. Never a source/producer/I/O grant.
#define PIONEER_Q38_PROJECTION_RESULT_STASH 1
#define PIONEER_Q38_PROJECTION_GRAPH_PLAN_FIXTURE_ONLY
#include "pioneer_q38_projection_graph_plan_test.cpp"
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 unsigned valid=0,refused=0,rows=0;
 const unsigned roles[]={10,11,5,6};
 for(unsigned slot=0;slot<4;slot++){
  unsigned role=roles[slot],offset=PioneerQ38ProjectionResultOffset(role),count=PioneerQ38ProjectionResultRows(role);
  assert(PioneerQ38ProjectionResultSlot(role)==slot && !(offset&4095) && offset>=TT_Q38_PROJECTION_RESULT_BASE
   && count && count*4<=TT_Q38_PROJECTION_RESULT_BASE+TT_Q38_PROJECTION_RESULT_BYTES-offset);
  for(unsigned previous=0;previous<slot;previous++)assert(offset>=PioneerQ38ProjectionResultOffset(roles[previous])+PioneerQ38ProjectionResultRows(roles[previous])*4);
  rows+=count;
 }
 for(unsigned layer=0;layer<=64;layer++)for(unsigned role=0;role<14;role++){
  unsigned t=0,k=0,n=0;auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
  bool allowed=!layer && PioneerQ38ProjectionResultSlot(role)<4;
  assert(PioneerQ38ProjectionOutputOffset(layer,role)==(allowed?PioneerQ38ProjectionResultOffset(role):UINT32_MAX));
  if(!plan->ProjectionGeometry(layer,role,t,k,n))continue;
  // Canonical ALL-model geometry remains present. New QPS1 does NOT
  // pretend that a first-layer embedding is a later-layer producer.
  for(unsigned bank=0;bank<7;bank++)for(bool stop:{false,true}){
   ParentPlanFixture f(layer,role,bank,stop);assert(f.Build()==allowed);
   if(!allowed){assert(!f.graph->Manifest());refused++;continue;}
   auto* m=const_cast<uint32_t*>(f.graph->Manifest());assert(m && f.Match() && m[0]==0x31535051 && n==PioneerQ38ProjectionResultRows(role));
   for(unsigned bad:{1U,3U,63U,64U,UINT32_MAX}){m[42]=bad;assert(!PioneerQ38ProjectionGraphFrame(m));refused++;}m[42]=0;
   for(unsigned bad:{0U,1U,2U,4U,13U,UINT32_MAX}){m[43]=bad;assert(!PioneerQ38ProjectionGraphFrame(m));refused++;}m[43]=role;
   m[0]=0x31525051;assert(!PioneerQ38ProjectionGraphFrame(m));m[0]=0x31535051;assert(f.Match());refused++;valid++;
  }
 }
 assert(rows==16480 && valid==56 && PioneerQ38ProjectionResultSlot(UINT32_MAX)==4
  && !PioneerQ38ProjectionResultRows(UINT32_MAX) && PioneerQ38ProjectionResultOffset(UINT32_MAX)==UINT32_MAX);
 printf("PASS QPS1 four distinct fixed page-aligned buffers/16480rows/18432words initialized prefix;ALL%u supported RUN+STOP bank shapes/%u wrong layer-role-marker refusals;canonical full-model geometry retained;NO_IO_CAPABILITY_OR_SPEED\n",valid,refused);
}
