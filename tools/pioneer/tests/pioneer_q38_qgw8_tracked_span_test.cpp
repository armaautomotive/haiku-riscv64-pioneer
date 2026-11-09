/* SPDX-License-Identifier: MIT */
// Actual span service + QGW8 receipt and emitted RV32 span code, with
// simulated fabric/banks. No driver, original weights or card access.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_QGW8_SPAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_TRACKED_SPAN_FIXTURE_ONLY
#include "pioneer_q38_hybrid_tracked_span_test.cpp"
#if defined(PIONEER_TEST_QGW8_REUSE) && (!defined(PIONEER_TEST_QGW8_FAST) \
 || !defined(PIONEER_Q38_SPAN_FAST) || !defined(PIONEER_Q38_SPAN_REUSE_CODE))
#error QGW8 reuse test requires the actual fast transport and code-reuse selection
#endif

static bool UploadQgw8(TrackedContext& c,tt_q38_span_upload& request,PioneerQ38Qgw8UploadReceipt* receipt){
#ifdef PIONEER_TEST_QGW8_FAST
 return PioneerQ38Qgw8SpanUploadFastInspection(c,request,receipt);
#else
 return PioneerQ38Qgw8SpanUploadInspection(c,request,receipt);
#endif
}
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 (void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 {
  TrackedContext c(argv[1]);int cookie=0;auto receipt=std::make_unique<PioneerQ38Qgw8UploadReceipt>();
  assert(receipt->Begin(&c,&cookie,c.hashes,c.initialized,17,1,c.banks,c.fixture.owner.dram,PioneerResidentBanks::Pages));
  unsigned bank=0,length=0;uint64_t offset=0;assert(receipt->Next(bank,offset,length));
  auto request=std::make_unique<tt_q38_span_upload>();request->version=1;request->bank=bank;request->offset=offset;request->bytes=length;
  for(unsigned i=0;i<length/4;i++)request->data[i]=0x7fc00000U+i*0x9e3779b9U;
  const unsigned touches=c.io.touches;
  assert(!UploadQgw8(c,*request,nullptr) && c.io.touches==touches);
  request->offset+=4096;
  assert(!UploadQgw8(c,*request,receipt.get()) && c.io.touches==touches);
  request->offset=offset;
  const unsigned wideLoads=c.io.wideLoads;
  assert(UploadQgw8(c,*request,receipt.get()) && receipt->Spans()==1 && c.uploadedPages==length/4096);
  const auto& result=request->result;
  assert(result.version==1 && result.state==TT_RUNTIME_PASSED && result.bytes==length && result.pages==length/4096
   && result.nonce==1 && result.started==1 && result.completed==1 && result.held==1
   && !result.recovery_required && !result.reserved);
#ifdef PIONEER_TEST_QGW8_FAST
  assert(!request->result.host_exact && !request->result.matched_words && c.io.wideLoads==wideLoads);
  assert(c.io.lastDeviceWords.size()==length/4 && std::equal(c.io.lastDeviceWords.begin(),c.io.lastDeviceWords.end(),request->data));
#else
  assert(request->result.host_exact==1 && request->result.matched_words==length/4 && c.io.wideLoads>wideLoads);
#endif
  const unsigned first=bank*unsigned(PioneerResidentBanks::Capacity/4096)+unsigned(offset/4096);
  for(unsigned p=0;p<length/4096;p++)assert(c.initialized[first+p]==1 && c.hashes[first+p]==PioneerResidentPageHash(request->data+p*1024));
  assert(PioneerQ38SpanEndService(c) && !receipt->Seal(*c.q38Span) && !receipt->Sealed());
  c.fixture.Finish();
 }
 {
  TrackedContext c(argv[1]);int cookie=0;auto receipt=std::make_unique<PioneerQ38Qgw8UploadReceipt>();
  assert(receipt->Begin(&c,&cookie,c.hashes,c.initialized,17,1,c.banks,c.fixture.owner.dram,PioneerResidentBanks::Pages));
  unsigned bank=0,length=0;uint64_t offset=0;assert(receipt->Next(bank,offset,length));
  auto request=std::make_unique<tt_q38_span_upload>();request->version=1;request->bank=bank;request->offset=offset;request->bytes=length;
  c.io.packetFault=1;
  assert(!UploadQgw8(c,*request,receipt.get()) && c.banks.Quarantined()
   && c.q38Span->unsafe && !receipt->Sealed() && !c.uploadedPages);
  const unsigned touches=c.io.touches;
  assert(!UploadQgw8(c,*request,receipt.get()) && c.io.touches==touches);
  c.fixture.Unsafe();
 }
#ifdef PIONEER_TEST_QGW8_REUSE
 {
  TrackedContext c(argv[1]);int cookie=0;auto receipt=std::make_unique<PioneerQ38Qgw8UploadReceipt>();
  assert(receipt->Begin(&c,&cookie,c.hashes,c.initialized,17,1,c.banks,c.fixture.owner.dram,PioneerResidentBanks::Pages));
  auto request=std::make_unique<tt_q38_span_upload>();
  for(unsigned job=0;job<2;job++){
   unsigned bank=0,length=0;uint64_t offset=0;assert(receipt->Next(bank,offset,length));
   *request={};request->version=1;request->bank=bank;request->offset=offset;request->bytes=length;
   for(unsigned i=0;i<length/4;i++)request->data[i]=0x7fc00000U+i*0x9e3779b9U+job;
   const unsigned before=c.io.codeLoads;
   assert(UploadQgw8(c,*request,receipt.get()) && receipt->Spans()==job+1);
   if(job==1)assert(c.io.codeLoads==before); // Work path reuses the initial code proof
  }
  assert(PioneerQ38SpanEndService(c) && c.q38Span->closed && !receipt->Seal(*c.q38Span));
  c.fixture.Finish();
 }
 {
  TrackedContext c(argv[1]);int cookie=0;auto receipt=std::make_unique<PioneerQ38Qgw8UploadReceipt>();
  assert(receipt->Begin(&c,&cookie,c.hashes,c.initialized,17,1,c.banks,c.fixture.owner.dram,PioneerResidentBanks::Pages));
  unsigned bank=0,length=0;uint64_t offset=0;assert(receipt->Next(bank,offset,length));
  auto request=std::make_unique<tt_q38_span_upload>();request->version=1;request->bank=bank;request->offset=offset;request->bytes=length;
  assert(UploadQgw8(c,*request,receipt.get()));
  c.io.view[(0x72000-TT_Q38_SPAN_VIEW_BASE)/4]^=1; // synthetic out-of-contract code mutation
  assert(!PioneerQ38SpanEndService(c) && !receipt->Sealed());
  c.fixture.Unsafe(); // no further IO, restore or release after uncertain END
 }
 puts("PASS QGW8 fast reuse two spans, zero repeated code reads, END tamper quarantine; SIMULATED ONLY");
#endif
 puts("PASS actual span service and QGW8 receipt: emitted RV32 simulated, ledger recorded, wrong order refused, fault quarantined; NO CARD");
}
