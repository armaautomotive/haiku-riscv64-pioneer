/* SPDX-License-Identifier: MIT */
// Exercise the production phase-switching adapter. Synthetic cache readers
// only; no accelerator, source allocation, inference or performance grant.
#define PIONEER_Q38_PREPARED_UPLOAD_FIXTURE_ONLY 1
#include "pioneer_q38_prepared_q4_upload_plan_test.cpp"
#include "../PioneerQ38PreparedQ4PlanningPages.h"
struct CaptureWeights:Weights {
 std::vector<bool> prepared;
 unsigned active=UINT32_MAX,readCalls=0,failPage=UINT32_MAX;
 bool complete=false,revoke=false;
 explicit CaptureWeights(const PioneerQwen38PackedPlan& p):Weights(p),prepared(p.Parts().size()){}
 bool AllPrepared()const{return complete && Weights::AllPrepared();}
 bool Correction(unsigned f,PioneerQ38NativeQ4WeightCache::CorrectionProposal& p)const{
  return f<prepared.size() && prepared[f] && Weights::Correction(f,p);
 }
 bool ReadPage(unsigned f,unsigned page,bool side,uint8_t* out,unsigned capacity,std::string& error){
  assert(f==active && side && capacity==4096);readCalls++;
  if(!ready || page==failPage){error="synthetic capture refusal";return false;}
  Pages actual(*this);
  if(!actual.ReadPage(f,page,side,out,capacity,error))return false;
  if(revoke){ready=false;return false;}return true;
 }
};
struct CapturePayload {
 CaptureWeights& weights;Pages actual;PioneerModelWeightCache::Ready source;
 bool bound=false,failBind=false;unsigned binds=0;
 explicit CapturePayload(CaptureWeights& w):weights(w),actual(w){}
 bool Bind(const char*,CaptureWeights& w,const PioneerModelWeightCache::Ready& ready,std::string&){
  binds++;if(failBind || bound || &w!=&weights || !w.AllPrepared() || !ready || !ready())return false;
  source=ready;bound=true;return true;
 }
 bool ReadyFor(const CaptureWeights& w)const{
  return bound && &w==&weights && w.AllPrepared() && source && source() && actual.ReadyFor(w);
 }
 bool ReadPage(unsigned f,unsigned p,bool side,uint8_t* out,unsigned capacity,std::string& error){
  return ReadyFor(weights) && actual.ReadPage(f,p,side,out,capacity,error);
 }
};
using Adapter=PioneerQ38PreparedQ4PlanningPages<CaptureWeights,CapturePayload>;
static void CaptureAll(Adapter& reader,CaptureWeights& weights,std::string& error){
 for(unsigned f=0;f<weights.entries.size();f++){
  if(!weights.entries[f].logical)continue;
  weights.active=f;weights.prepared[f]=true;assert(reader.Capture(weights,f,error));
 }
 weights.complete=true;
}
int main(){
 setvbuf(stdout,nullptr,_IOLBF,0);Fixture fixture;PioneerQwen38Gguf model;assert(model.Open(fixture.file));
 PioneerQwen38PackedPlan original;assert(original.Build(model,8192,true,true));std::string error;
 CaptureWeights weights(original);CapturePayload payload(weights);Adapter pages(payload);
 std::array<uint8_t,4096> out;out.fill(0x93);const auto prior=out;unsigned refusals=0;
 assert(!pages.ReadPage(0,0,true,out.data(),out.size(),error) && out==prior);refusals++;
 assert(!pages.FinishPlanning());refusals++;
 CaptureAll(pages,weights,error);assert(pages.CapturedFragments()==1344 && !payload.actual.reads);
 assert(pages.Bind("unused",weights,[]{return true;},error) && pages.ReadyFor(weights));
 assert(!pages.Bind("unused",weights,[]{return true;},error) && payload.binds==1);refusals++;
 unsigned first=0;while(!weights.entries[first].logical)first++;
 assert(!pages.Capture(weights,first,error));refusals++;
 for(auto* alias:{reinterpret_cast<uint8_t*>(&pages),reinterpret_cast<uint8_t*>(&weights),
  reinterpret_cast<uint8_t*>(&payload),reinterpret_cast<uint8_t*>(const_cast<PioneerQ38PackedPart*>(original.Parts().data())),
  reinterpret_cast<uint8_t*>(UINTPTR_MAX-1024)}){
  assert(!pages.ReadPage(first,0,true,alias,4096,error));refusals++;
 }
 assert(!pages.ReadPage(first,0,false,out.data(),out.size(),error) && out==prior);refusals++;
 assert(!pages.ReadPage(first,0,true,out.data(),4095,error) && out==prior);refusals++;
 assert(!pages.ReadPage(first,UINT32_MAX,true,out.data(),out.size(),error) && out==prior);refusals++;
 using Planned=PioneerQ38PreparedQ4UploadPlan<CaptureWeights,Adapter>;Planned plan;
 assert(plan.Build(weights,pages,error) && plan.Ready() && plan.Spans().size()==4895
  && pages.PlanningReads()==pages.CapturedPages() && !payload.actual.reads);
 // The planning-only tail is zero, NOT the actual correction-mask payload.
 const auto& entry=weights.entries[first];const unsigned page=entry.indexBytes/4096;
 assert(pages.ReadPage(first,page,true,out.data(),out.size(),error));
 assert(out[entry.indexBytes%4096]==0 && !payload.actual.reads);
 assert(pages.FinishPlanning() && plan.Ready() && pages.ReadyFor(weights));
 assert(!pages.FinishPlanning());refusals++;
 assert(pages.ReadPage(first,page,true,out.data(),out.size(),error)
  && out[entry.indexBytes%4096]==0xa7 && payload.actual.reads==1);
 assert(pages.ReadPage(first,0,false,out.data(),out.size(),error) && out[0]==0xa7 && payload.actual.reads==2);
 const unsigned planningReads=unsigned(pages.PlanningReads());
 auto raw=[](unsigned,unsigned,uint8_t* p,unsigned n){memset(p,0,n);return true;};
 unsigned runtime=0;
 for(unsigned s=0;s<plan.Spans().size();s++)if(plan.Spans()[s].kind!=Planned::Original){
  assert(plan.ReadPage(s,0,out.data(),out.size(),raw,error));runtime++;
 }
 assert(runtime==2688 && pages.PlanningReads()==planningReads && payload.actual.reads==runtime+2);
 payload.actual.ready=false;out.fill(0x93);
 assert(!pages.ReadPage(first,0,true,out.data(),out.size(),error) && out==prior && !plan.Ready());refusals++;
 payload.actual.ready=true;weights.ready=false;
 assert(!pages.ReadPage(first,0,true,out.data(),out.size(),error) && out==prior && !plan.Ready());refusals++;
 weights.ready=true;
 {CapturePayload p(weights);Adapter a(p);
  assert(!a.Bind("unused",weights,[]{return true;},error) && !a.ReadyFor(weights));
  assert(!a.Capture(weights,first,error) && !a.Bind("unused",weights,[]{return true;},error));refusals+=3;}
 {CapturePayload p(weights);Adapter a(p);weights.active=first;
  assert(a.Capture(weights,first,error) && !a.Capture(weights,first,error) && !a.ReadyFor(weights));
  assert(!a.Bind("unused",weights,[]{return true;},error));refusals+=2;}
 {CapturePayload p(weights);Adapter a(p);
  assert(!a.Capture(weights,unsigned(original.Parts().size()),error) && !a.Capture(weights,first,error));refusals+=2;}
 unsigned wide=first;while(weights.entries[wide].indexBytes<=4096)wide++;
 for(bool withdraw:{false,true}){
  CapturePayload p(weights);Adapter a(p);weights.active=wide;weights.failPage=withdraw?UINT32_MAX:1;weights.revoke=withdraw;
  assert(!a.Capture(weights,wide,error) && !a.CapturedFragments() && !a.ReadyFor(weights));
  weights.ready=true;weights.failPage=UINT32_MAX;weights.revoke=false;
  assert(!a.Capture(weights,wide,error));refusals+=2;
 }
 for(unsigned failure=0;failure<2;failure++){
  CapturePayload p(weights);Adapter a(p);CaptureAll(a,weights,error);p.failBind=failure==1;
  assert(!a.Bind("unused",weights,[failure]{return failure!=0;},error) && !a.ReadyFor(weights));refusals++;
 }
 printf("PASS_PREPARED_Q4_PLANNING_PAGES capture1344 planningOnlyReads%u payloadPlanningReads0 runtimeDelegates%u "
  "phaseSwitchOneWay aliasesRefused atomicCaptureFailure sourceWithdrawalRefused refusals%u;SYNTHETIC_NO_DEVICE_OR_TPS\n",
  planningReads,runtime+2,refusals);
}
