/* SPDX-License-Identifier: MIT */
// Existing span core with relocated receipt; simulated OS and card only.
#define PIONEER_Q38_SMALL_RECURRENT_BFP8_PLAN_INSPECTION 1
#define PIONEER_Q38_SMALL_RECURRENT_SPAN_INSPECTION 1
#define PIONEER_Q38_RECURRENT_Q8_CONTROL_INSPECTION 1
#include "pioneer_q38_recurrent_q8_span_test.cpp"
#undef PIONEER_BANKS_MOCK_ONLY
#define PIONEER_Q38_SMALL_RECURRENT_UPLOAD_PLAN_FIXTURE_ONLY 1
#include "pioneer_q38_small_recurrent_upload_plan_test.cpp"

int main(int argc,char** argv){
 assert(argc==2);
 for(unsigned mode=0;mode<3;mode++){
  PioneerProbeCookie cookie;NativeSpanContext c(argv[1]);Bind(cookie,c);c.q38Span=new PioneerQ38SpanLease;
  auto receipt=std::make_unique<PioneerQ38SmallRecurrentUploadReceiptInspection>();
  assert(receipt->Begin(&c,&cookie,c.hashes,c.initialized,cookie.team,sReloadGeneration,
   c.banks,c.baseline.baseline.baseline.baseline.dram,unsigned(c.bytes/4096)));
  unsigned bank=0,length=0;uint64_t offset=0;assert(receipt->Next(bank,offset,length));
  auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
  FakePrepared prepared;assert(FakePrepared::Plan::Build(*canonical,prepared.proposal));
  PioneerQ38SmallRecurrentUploadPlan<FakePrepared> stage;std::string error;assert(stage.Bind(*canonical,prepared,error));
  auto request=Request(length,bank,offset);unsigned fragment=0,stagedBank=0,stagedLength=0;uint64_t stagedOffset=0;
  assert(stage.Stage(reinterpret_cast<uint8_t*>(request->data),sizeof(request->data),fragment,stagedBank,stagedOffset,stagedLength,error));
  assert(stagedBank==bank && stagedOffset==offset && stagedLength==length);
  if(mode==2)c.io.packetFault=1;
  const bool okay=mode==0?PioneerQ38SmallRecurrentSpanUploadInspection(c,*request,receipt.get())
   :PioneerQ38SmallRecurrentSpanUploadFastInspection(c,*request,receipt.get());
  if(mode<2){
   assert(okay && request->result.state==TT_RUNTIME_PASSED && receipt->Spans()==1
    && c.uploadedPages==length/4096 && !c.Unsafe());
   assert(stage.Acknowledge(true));
   const unsigned touches=c.io.touches;
   assert(!PioneerQ38SmallRecurrentSpanUploadFastInspection(c,*request,receipt.get())
    && c.io.touches==touches && receipt->Spans()==1);
  }else assert(!okay && c.banks.Quarantined() && receipt->Spans()==0 && !stage.Acknowledge(false));
  Unbind();
 }
 puts("PASS relocated receipt with real span service, normal/Work transport and failed-transfer quarantine; SIMULATED NO DEVICE");
}
