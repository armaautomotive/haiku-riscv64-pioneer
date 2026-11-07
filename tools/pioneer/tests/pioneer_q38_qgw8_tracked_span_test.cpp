/* SPDX-License-Identifier: MIT */
// Actual span service + QGW8 receipt and emitted RV32 span code, with
// simulated fabric/banks. No driver, original weights or card access.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_QGW8_SPAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_TRACKED_SPAN_FIXTURE_ONLY
#include "pioneer_q38_hybrid_tracked_span_test.cpp"

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
  assert(!PioneerQ38Qgw8SpanUploadInspection(c,*request,nullptr) && c.io.touches==touches);
  request->offset+=4096;
  assert(!PioneerQ38Qgw8SpanUploadInspection(c,*request,receipt.get()) && c.io.touches==touches);
  request->offset=offset;
  assert(PioneerQ38Qgw8SpanUploadInspection(c,*request,receipt.get()) && receipt->Spans()==1 && c.uploadedPages==length/4096);
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
  assert(!PioneerQ38Qgw8SpanUploadInspection(c,*request,receipt.get()) && c.banks.Quarantined()
   && c.q38Span->unsafe && !receipt->Sealed() && !c.uploadedPages);
  const unsigned touches=c.io.touches;
  assert(!PioneerQ38Qgw8SpanUploadInspection(c,*request,receipt.get()) && c.io.touches==touches);
  c.fixture.Unsafe();
 }
 puts("PASS actual span service and QGW8 receipt: emitted RV32 simulated, ledger recorded, wrong order refused, fault quarantined; NO CARD");
}
