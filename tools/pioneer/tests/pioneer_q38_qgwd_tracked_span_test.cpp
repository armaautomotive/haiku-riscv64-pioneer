/* SPDX-License-Identifier: MIT */
// Actual span service, QGWD gapped receipt, and emitted RV32 span image;
// all transport and banks are synthetic. No card, model, driver or inference.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION 1
#define PIONEER_Q38_QGWD_SPAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_TRACKED_SPAN_FIXTURE_ONLY
#include "pioneer_q38_hybrid_tracked_span_test.cpp"
#ifdef PIONEER_TEST_QGWD_WRONG_PAIR
bool RefuseWrongPair(TrackedContext& c,tt_q38_span_upload& request,PioneerQ38DenseQ8UploadReceiptInspection* receipt){
 return PioneerQ38SpanUploadCore<TrackedContext,true,PioneerQ38DenseQ8UploadReceiptInspection,false>(c,request,receipt);
}
#endif

static bool UploadQgwd(TrackedContext& c,tt_q38_span_upload& request,PioneerQ38DenseQ8UploadReceiptInspection* receipt){
#ifdef PIONEER_TEST_QGWD_FAST
 return PioneerQ38QgwdSpanUploadFastInspection(c,request,receipt);
#else
 return PioneerQ38QgwdSpanUploadInspection(c,request,receipt);
#endif
}
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 (void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 {
  TrackedContext c(argv[1]);int cookie=0;auto receipt=std::make_unique<PioneerQ38DenseQ8UploadReceiptInspection>();
  assert(receipt->Begin(&c,&cookie,c.hashes,c.initialized,17,1,c.banks,c.fixture.owner.dram,PioneerResidentBanks::Pages));
  unsigned bank=0,length=0;uint64_t offset=0;assert(receipt->Next(bank,offset,length));
  auto request=std::make_unique<tt_q38_span_upload>();request->version=1;request->bank=bank;request->offset=offset;request->bytes=length;
  for(unsigned i=0;i<length/4;i++)request->data[i]=0x7fc00000U+i*0x9e3779b9U;
  const unsigned touches=c.io.touches;
  assert(!UploadQgwd(c,*request,nullptr) && c.io.touches==touches);
  request->offset+=4096;assert(!UploadQgwd(c,*request,receipt.get()) && c.io.touches==touches);request->offset=offset;
  const unsigned wideLoads=c.io.wideLoads;
  assert(UploadQgwd(c,*request,receipt.get()) && receipt->Spans()==1 && c.uploadedPages==length/4096);
  const auto& result=request->result;
  assert(result.version==1 && result.state==TT_RUNTIME_PASSED && result.bytes==length && result.pages==length/4096
   && result.nonce==1 && result.started==1 && result.completed==1 && result.held==1 && !result.recovery_required && !result.reserved);
#ifdef PIONEER_TEST_QGWD_FAST
  assert(!result.host_exact && !result.matched_words && c.io.wideLoads==wideLoads);
  assert(c.io.lastDeviceWords.size()==length/4 && std::equal(c.io.lastDeviceWords.begin(),c.io.lastDeviceWords.end(),request->data));
#else
  assert(result.host_exact==1 && result.matched_words==length/4 && c.io.wideLoads>wideLoads);
#endif
  const unsigned first=bank*unsigned(PioneerResidentBanks::Capacity/4096)+unsigned(offset/4096);
  for(unsigned p=0;p<length/4096;p++)assert(c.initialized[first+p]==1 && c.hashes[first+p]==PioneerResidentPageHash(request->data+p*1024));
  assert(PioneerQ38SpanEndService(c) && !receipt->Seal(*c.q38Span) && !receipt->Sealed());c.fixture.Finish();
 }
 {
  // Advance the receipt with synthetic, already-published ledger pages to
  // the first converted dense stripe; then exercise the REAL span service at
  // its compact address. The skipped calls do not represent device I/O.
  TrackedContext c(argv[1]);int cookie=0;auto receipt=std::make_unique<PioneerQ38DenseQ8UploadReceiptInspection>();
  auto original=std::make_unique<PioneerQ38CanonicalModelPlan>();auto parent=std::make_unique<PioneerQ38Qgw8KernelLayout>();
  auto layout=std::make_unique<PioneerQ38DenseQ8KernelLayoutInspection>();
  assert(original->Build() && parent->Build(*original) && layout->Build(*original,*parent));
  const PioneerQ38Qgw8KernelLayout::Part* dense=nullptr;const PioneerQ38Qgw8KernelLayout::Part* prior=nullptr;
  for(unsigned i=0;i<original->Count();i++)if(layout->At(i)->encoding==PioneerQ38DenseQ8KernelLayoutInspection::DenseQ8Bfp8){
   dense=layout->At(i);prior=parent->At(i);break;}
  assert(dense && prior && dense->allocated<prior->allocated);
  assert(receipt->Begin(&c,&cookie,c.hashes,c.initialized,17,1,c.banks,c.fixture.owner.dram,PioneerResidentBanks::Pages));
  auto request=std::make_unique<tt_q38_span_upload>();unsigned bank=0,length=0;uint64_t offset=0;
  assert(receipt->Next(bank,offset,length));request->version=1;request->bank=bank;request->offset=offset;request->bytes=length;
  assert(UploadQgwd(c,*request,receipt.get()));unsigned skipped=0;
  while(receipt->Next(bank,offset,length) && !(bank==dense->bank && offset==dense->offset)){
   assert(receipt->Before(&c,c.hashes,c.initialized,c.q38Span,bank,offset,length));
   const unsigned first=bank*unsigned(PioneerResidentBanks::Capacity/4096)+unsigned(offset/4096);
   for(unsigned p=0;p<length/4096;p++){const unsigned index=first+p;c.hashes[index]=index*2654435761U;
    c.initialized[index]=1;assert(receipt->RecordPage(index,c.hashes[index]));}
   c.q38Span->nonce++;assert(receipt->After());skipped++;
  }
  assert(skipped && bank==dense->bank && offset==dense->offset);*request={};request->version=1;
  request->bank=bank;request->offset=offset;request->bytes=length;
  for(unsigned i=0;i<length/4;i++)request->data[i]=0x3f800000U+i;
  assert(UploadQgwd(c,*request,receipt.get()));
  const unsigned gap=bank*unsigned(PioneerResidentBanks::Capacity/4096)+(dense->offset+dense->allocated)/4096;
  assert(!c.initialized[gap] && !c.hashes[gap]);
  assert(PioneerQ38SpanEndService(c) && !receipt->Seal(*c.q38Span));c.fixture.Finish();
 }
 {
  TrackedContext c(argv[1]);int cookie=0;auto receipt=std::make_unique<PioneerQ38DenseQ8UploadReceiptInspection>();
  assert(receipt->Begin(&c,&cookie,c.hashes,c.initialized,17,1,c.banks,c.fixture.owner.dram,PioneerResidentBanks::Pages));
  unsigned bank=0,length=0;uint64_t offset=0;assert(receipt->Next(bank,offset,length));
  auto request=std::make_unique<tt_q38_span_upload>();request->version=1;request->bank=bank;request->offset=offset;request->bytes=length;
  c.io.packetFault=1;assert(!UploadQgwd(c,*request,receipt.get()) && c.banks.Quarantined()
   && c.q38Span->unsafe && !receipt->Sealed() && !c.uploadedPages);
  const unsigned touches=c.io.touches;assert(!UploadQgwd(c,*request,receipt.get()) && c.io.touches==touches);c.fixture.Unsafe();
 }
 puts("PASS actual span service with QGWD gapped receipt, emitted RV32 simulation, out-of-order refusal and fault quarantine; NO CARD");
}
