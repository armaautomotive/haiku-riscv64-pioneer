/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 1000
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD 1
#define PIONEER_Q38_SPAN_SERVICE_MOCK_ONLY 1
#include "pioneer_q38_span_controller_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanService.h"
#include <memory>
struct SpanRef1 {tt_dram_samples_result& dram;};
struct SpanRef2 {SpanRef1 baseline;tt_compute_noc_result& noc;};
struct SpanRef3 {SpanRef2 baseline;};struct SpanRef4 {SpanRef3 baseline;};
struct TrackedContext {
 SpanControlFixture fixture;SpanControlIO& io;PioneerResidentBanks& banks;
 tt_host_page_result& host;PioneerResidentLease& lease;PioneerBulkUploadLease& bulk;
 PioneerOutputBufferLease& bulkHost;PioneerWideOutputLease& wideHost;PioneerQ38SpanLease* q38Span;
 void* q38Dispatch=nullptr;void* q38Cohort=nullptr;void* fleet=nullptr;
 struct {bool saved=false;} panelProbe,streamProbe;
 int runtimeCoreMode=-1;bool bankStorageOnly=true;uint64_t bytes=PioneerResidentBanks::Total;
 std::vector<uint32_t> hashWords=std::vector<uint32_t>(PioneerResidentBanks::Pages,0);
 uint8_t* initialized;uint32_t* hashes;uint32_t uploadedPages=0;SpanRef4 baseline;
 explicit TrackedContext(const char* image):fixture(image),io(fixture.io),banks(fixture.owner.banks),host(fixture.owner.host),
  lease(fixture.owner.store),bulk(fixture.bulk),bulkHost(fixture.transfer),wideHost(fixture.wide),q38Span(&fixture.lease),
  initialized(fixture.initialized.data()),hashes(hashWords.data()),baseline{{{{fixture.owner.dram},fixture.owner.noc}}}{}
 bool Unsafe(){return banks.Quarantined() || q38Span->unsafe;}
};
#ifndef PIONEER_Q38_HYBRID_TRACKED_SPAN_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 TrackedContext c(argv[1]);int cookie=0;auto receipt=std::make_unique<PioneerQ38HybridUploadReceipt>();
 assert(receipt->Begin(&c,&cookie,c.hashes,c.initialized,17,1,c.banks,c.fixture.owner.dram,PioneerResidentBanks::Pages));
 unsigned bank=0,length=0;uint64_t offset=0;assert(receipt->Next(bank,offset,length));
 auto request=std::make_unique<tt_q38_span_upload>();request->version=1;request->bank=bank;request->offset=offset;request->bytes=length;
 for(unsigned i=0;i<length/4;i++)request->data[i]=0x7fc00000U+i*0x9e3779b9U;
 const unsigned touches=c.io.touches;
 assert(!PioneerQ38SpanUploadService(c,*request,nullptr) && c.io.touches==touches);
 request->offset+=4096;assert(!PioneerQ38SpanUploadService(c,*request,receipt.get()) && c.io.touches==touches);request->offset=offset;
 assert(PioneerQ38SpanUploadService(c,*request,receipt.get()) && receipt->Spans()==1 && c.uploadedPages==length/4096);
 const unsigned first=bank*unsigned(PioneerResidentBanks::Capacity/4096)+unsigned(offset/4096);
 for(unsigned p=0;p<length/4096;p++)assert(c.initialized[first+p]==1 && c.hashes[first+p]==PioneerResidentPageHash(request->data+p*1024));
 assert(PioneerQ38SpanEndService(c) && !receipt->Seal(*c.q38Span) && !receipt->Sealed());
 c.fixture.Finish();puts("PASS actual tracked span service publishes page hashes into same receipt; wrong order/null refused before IO; NO CARD");
}
#endif
