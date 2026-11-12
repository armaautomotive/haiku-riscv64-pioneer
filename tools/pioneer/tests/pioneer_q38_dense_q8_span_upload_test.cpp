/* SPDX-License-Identifier: MIT */
// Exact span packet construction and failure behavior with synthetic pages.
// This test does not open a device, verify model data or authorize a launch.
#define PIONEER_Q38_HYBRID_SPAN_FIXTURE_ONLY 1
#define PIONEER_Q38_HYBRID_HEAD_UPLOAD_DIAGNOSTIC 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_UPLOAD_INSPECTION 1
#include "pioneer_q38_hybrid_span_upload_test.cpp"
#include "../PioneerQ38UploadTiming.h"
using Upload=PioneerQ38HybridSpanUpload<FakePages,FakeIO,PioneerQ38DenseQ8UploadPlan>;
static uint64_t clockTick=1;
static uint64_t TestClock(){return clockTick++;}
static void SetPlan(FakePages& pages,const Canonical& canonical){
 PioneerQ38DenseQ8UploadPlan::Proposal plan;assert(PioneerQ38DenseQ8UploadPlan::Build(canonical,plan));
 pages.proposal.parts=std::move(plan.parts);pages.proposal.used=plan.used;
}
int main(){
 auto canonical=std::make_unique<Canonical>();assert(canonical->Build());std::string error;
 {
  FakePages pages(*canonical);SetPlan(pages,*canonical);FakeIO io{*canonical,pages};Upload upload;
  unsigned index=0;while(index<pages.proposal.parts.size() && pages.proposal.parts[index].encoding!=10)index++;
  assert(index<pages.proposal.parts.size());pages.proposal.parts[index].encoding=0;
  assert(!upload.Run(pages,*canonical,io,42,error) && upload.Failed() && !upload.DeviceTouched() && !io.calls);
 }
 {
  FakePages pages(*canonical);SetPlan(pages,*canonical);FakeIO io{*canonical,pages};Upload upload;io.failCall=2;
  assert(!upload.Run(pages,*canonical,io,42,error) && upload.Failed() && upload.DeviceTouched() && io.calls==2);
  assert(!upload.Run(pages,*canonical,io,42,error) && io.calls==2);
 }
 {
  FakePages pages(*canonical);SetPlan(pages,*canonical);FakeIO io{*canonical,pages};Upload upload;uint64_t planned=0;
  for(unsigned i=0;i<canonical->Count();i++)if(canonical->Part(i)->kind==Canonical::WeightKind)planned+=pages.proposal.parts[i].allocated;
  unsigned progress=0;assert(upload.Run(pages,*canonical,io,42,error,[&](uint64_t done,uint64_t total){
   assert(done==io.transferred && total==planned);progress++;
  }));
  assert(upload.CompleteHeld() && !upload.Failed() && upload.UploadedParts()==3551 && upload.UploadedBytes()==planned
   && upload.PlannedBytes()==planned && io.calls==upload.UploadedSpans() && progress==io.calls && pages.reads==planned/4096);
  printf("PASS synthetic dense-Q8 upload: %u fragments, %u span packets, %llu bytes; no hardware or model claim\n",
   upload.UploadedParts(),upload.UploadedSpans(),(unsigned long long)planned);
 }
 {
  FakePages pages(*canonical);SetPlan(pages,*canonical);FakeIO io{*canonical,pages};
  PioneerQ38UploadTiming timing(TestClock);PioneerQ38TimedPages<FakePages> wrapped(pages,timing);
  PioneerQ38HybridSpanUpload<PioneerQ38TimedPages<FakePages>,FakeIO,PioneerQ38DenseQ8UploadPlan> upload;
  assert(upload.Run(wrapped,*canonical,io,42,error) && upload.CompleteHeld());
  assert(timing.valid && timing.readCalls && timing.readNs==timing.readCalls
   && timing.readBytes==upload.UploadedBytes() && upload.UploadedSpans()==21907 && pages.reads==5149957);
  puts("PASS timed wrapper preserves all QGWD upload packets and exact bytes; synthetic transport only");
 }
}
