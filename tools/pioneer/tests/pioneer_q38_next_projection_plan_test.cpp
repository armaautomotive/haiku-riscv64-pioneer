/* SPDX-License-Identifier: MIT */
// QPS2 DATA factories coexist with old QPR1 without retagging a native owner.
#define PIONEER_Q38_PROJECTION_GRAPH_PLAN_FIXTURE_ONLY
#include "pioneer_q38_projection_graph_plan_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextProjectionGraphPlan.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextProjectionGraphProof.h"
static_assert(!std::is_copy_constructible<PioneerQ38NextProjectionGraphPlan>::value
 && !std::is_move_constructible<PioneerQ38NextProjectionGraphPlan>::value,"private borrowed DATA heap factory");
int main(){
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;setvbuf(stdout,nullptr,_IOLBF,0);
 ParentPlanFixture f(1,10);unsigned good=0,refused=0,mutations=0;
 for(unsigned layer=0;layer<=64;layer++)for(unsigned role=0;role<14;role++){
  unsigned type=0,k=0,n=0;const bool allowed=layer>0 && layer<64 && layer%4!=3 && (role==5 || role==6 || role==10 || role==11);
  assert(PioneerQ38NextProjectionGeometry(layer,role,type,k,n)==allowed);
  if(!allowed){assert(PioneerQ38NextProjectionOutputOffset(layer,role)==~uint32_t(0));refused++;continue;}
  assert(type==8 && k==5120 && n==PioneerQ38ProjectionResultRows(role));
  unsigned originalType=0,originalK=0,originalN=0;assert(f.canonical->ProjectionGeometry(layer,role,originalType,originalK,originalN)
   && type==originalType && k==originalK && n==originalN);
  auto jobs=std::make_unique<PioneerQ38ProjectionJobs>();assert(jobs->Build(*f.canonical,layer,role,f.token));
  for(unsigned bank=0;bank<7;bank++)for(bool stop:{false,true}){
   auto p=std::make_unique<PioneerQ38NextProjectionGraphPlan>();uint64_t epoch=stop?0:f.input;
   assert(!p->Manifest() && p->Build(*f.canonical,*jobs,layer,role,f.token,bank,f.id,f.root,f.sources,f.sequence,f.phase,epoch,stop)
    && p->Matches(*f.canonical,*jobs,layer,role,f.token,bank,f.id,f.root,f.sources,f.sequence,f.phase,epoch,stop)
    && !p->Build(*f.canonical,*jobs,layer,role,f.token,bank,f.id,f.root,f.sources,f.sequence,f.phase,epoch,stop));
   const auto* m=p->Manifest();assert(m && m[0]==0x32535051 && m[1]==2 && m[42]==layer && m[43]==role
    && PioneerQ38NextProjectionGraphManifest(m,p->Sources()) && !PioneerQ38ProjectionGraphFrame(m));
   assert(PioneerQ38NextProjectionOutputOffset(layer,role)+n*4<=TT_Q38_PROJECTION_RESULT_BASE+TT_Q38_PROJECTION_RESULT_BYTES);
   for(unsigned w=0;w<119;w++)assert(PioneerQ38NextProjectionGraphNextPeer(m,w)==f.sequence[w]+(stop?1:unsigned(w<20)+unsigned(w<jobs->Count())));
   auto* mutableM=const_cast<uint32_t*>(m);
   for(unsigned word:{0U,1U,3U,6U,13U,32U,40U,42U,43U,44U,45U,46U,48U,63U,64U,182U,183U,191U,192U,195U,8383U}){
    mutableM[word]^=1;assert(!p->Matches(*f.canonical,*jobs,layer,role,f.token,bank,f.id,f.root,f.sources,f.sequence,f.phase,epoch,stop));mutableM[word]^=1;mutations++;
   }
   PioneerQ38NextProjectionGraphTraffic traffic;assert(PioneerQ38NextProjectionGraphExpectedTraffic(m,traffic));
   good++;
  }
 }
 // Caller-ready bits,layer0/attention/vocabulary are not expanded by the new grammar.
 for(unsigned layer:{0U,3U,63U,64U}){
  auto jobs=std::make_unique<PioneerQ38ProjectionJobs>();unsigned role=layer==64?0:10;assert(jobs->Build(*f.canonical,layer,role,f.token));
  auto p=std::make_unique<PioneerQ38NextProjectionGraphPlan>();assert(!p->Build(*f.canonical,*jobs,layer,role,f.token,0,f.id,f.root,f.sources,f.sequence,f.phase,f.input,false)
   && !p->Manifest() && !p->Sources());refused++;
 }
 assert(good==47*4*7*2);
 printf("PASS QPS2 independent namespace/coexisting QPR1/all47nonzero-linear layers/4roles/7banks/RUN+STOP=%u/%urefusals/%uactual canonical binding mutations;NO_IO_NATIVE_PRODUCER_OR_SPEED\n",good,refused,mutations);
}
