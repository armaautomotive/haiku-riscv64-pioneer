/* SPDX-License-Identifier: MIT */
// The actual QGWD control, span core and gapped receipt under simulated OS,
// banks and RV32 transport. No real driver, card, source model or inference.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_QGW8_SPAN_INSPECTION 1
#define PIONEER_Q38_QGW8_CONTROL_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_LEDGER_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_UPLOAD_RECEIPT_INSPECTION 1
#define PIONEER_Q38_QGWD_SPAN_INSPECTION 1
#define PIONEER_Q38_QGWD_CONTROL_INSPECTION 1
#define PIONEER_Q38_QGWD_UPLOAD_SESSION_INSPECTION 1
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_SPAN_FAST 1
#include "pioneer_q38_span_service_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8UploadProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38QgwdNativeBinding.h"
// Exercise the unmodified native boundary function in this isolated fixture.
// The temporary selector is not used to include or authorize any driver.
#define PioneerStoreContext NativeSpanContext
#define PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION 1
#include "pioneer_qgwd_native_coverage.inc"
#undef PIONEER_Q38_QGWD_RECEIPT_JOURNAL_INSPECTION
#undef PioneerStoreContext
static int qgwd_fixture_geteuid(){return 0;}
struct JournalProbe {
 bool called=false;
 bool BindDenseReceiptInspection(PioneerQ38DenseQ8UploadReceiptInspection& receipt,const void* context,
  const PioneerQ38DispatchLease* cohort,const void* cookie,const uint32_t* hashes,int64_t team,uint64_t generation){
  called=true;return context && cohort && receipt.Matches(context,cookie,hashes,
   static_cast<const NativeSpanContext*>(context)->initialized,team,generation);
 }
};
#define geteuid qgwd_fixture_geteuid
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38QgwdUploadControlInspection.inc"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8UploadControlInspection.inc"
#undef geteuid
// Extracted from the real ProbeControl dispatcher; synthetic OS only.
static status_t QgwdDispatch(PioneerProbeCookie* instance,uint32 operation,void* data,size_t length){
#include "pioneer_qgwd_dispatch.inc"
 return B_NOT_ALLOWED;
}
static status_t QgwdControl(PioneerProbeCookie* cookie,void* data,size_t length){
 return QgwdDispatch(cookie,TT_PCI_PROBE_Q38_QGWD_UPLOAD_CONTROL,data,length);
}

static void FillRemainingSynthetic(NativeSpanContext& c){
 unsigned bank=0,length=0;uint64_t offset=0;
 while(c.q38QgwdReceipt->Next(bank,offset,length)){
  assert(c.q38QgwdReceipt->Before(&c,c.hashes,c.initialized,c.q38Span,bank,offset,length));
  const unsigned first=bank*unsigned(PioneerResidentBanks::Capacity/4096)+unsigned(offset/4096);
  for(unsigned i=0;i<length/4096;i++){const unsigned index=first+i;const uint32_t hash=index*2654435761U;
   c.initialized[index]=1;c.hashes[index]=hash;assert(c.q38QgwdReceipt->RecordPage(index,hash));}
  c.q38Span->saved=c.q38Span->ready=c.q38Span->dirty=true;c.q38Span->nonce++;
  assert(c.q38QgwdReceipt->After());
 }
 assert(c.q38QgwdReceipt->Spans()==21907);
}
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&Request;
 (void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&Q38HybridUploadControl;(void)&Q38SpanControl;
 namespace D=PioneerQ38QgwdUpload;namespace W=PioneerQ38Qgw8Upload;
 static_assert(TT_PCI_PROBE_Q38_QGWD_UPLOAD_CONTROL!=TT_PCI_PROBE_Q38_QGW8_UPLOAD_CONTROL,"distinct ioctl");
 for(unsigned i=0;i<8;i++){D::Packet p;p.operation=D::Begin;
  uint32_t* fields[]={&p.magic,&p.version,&p.operation,&p.reserved,&p.accepted,&p.sealed,&p.spans,&p.recovery_required};
  *fields[i]^=1;assert(!D::Request(p));}
 PioneerProbeCookie cookie,foreign;
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);D::Packet p;p.operation=D::Begin;
  W::Packet rejected;rejected.operation=W::Begin;
  assert(Q38Qgw8UploadControlInspection(&cookie,&rejected,sizeof(rejected))==B_NOT_ALLOWED && !c.q38Qgw8Receipt);
  PioneerQ38HybridUpload::Packet rejectedHybrid;rejectedHybrid.operation=PioneerQ38HybridUpload::Begin;
  assert(Q38HybridUploadControl(&cookie,&rejectedHybrid,sizeof(rejectedHybrid))==B_NOT_ALLOWED && !c.q38HybridReceipt);
  W::Packet old;old.operation=W::Begin;std::memcpy(&p,&old,sizeof(p));
  assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==B_BAD_VALUE && !c.q38QgwdReceipt);
  p=D::Packet{};p.operation=D::Begin;team=18;
  assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==B_NOT_ALLOWED);team=17;
  assert(Q38QgwdUploadControlInspection(&foreign,&p,sizeof(p))==B_BUSY && !c.q38QgwdReceipt);
  c.q38Qgw8Receipt=new PioneerQ38Qgw8UploadReceipt;
  assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==B_NOT_ALLOWED && !c.q38QgwdReceipt);
  delete c.q38Qgw8Receipt;c.q38Qgw8Receipt=nullptr;
  c.q38HybridReceipt=new PioneerQ38HybridUploadReceipt;
  assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==B_NOT_ALLOWED && !c.q38QgwdReceipt);
  delete c.q38HybridReceipt;c.q38HybridReceipt=nullptr;
  assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==B_OK && p.accepted && !p.sealed && !p.spans);
  W::Packet other;other.operation=W::Begin;
  assert(Q38Qgw8UploadControlInspection(&cookie,&other,sizeof(other))==B_NOT_ALLOWED && !c.q38Qgw8Receipt);
  PioneerQ38HybridUpload::Packet hybrid;hybrid.operation=PioneerQ38HybridUpload::Begin;
  assert(Q38HybridUploadControl(&cookie,&hybrid,sizeof(hybrid))==B_NOT_ALLOWED && !c.q38HybridReceipt);
  p=D::Packet{};p.operation=D::Begin;assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==B_NOT_ALLOWED);
  unsigned bank=0,length=0;uint64_t offset=0;assert(c.q38QgwdReceipt->Next(bank,offset,length));
  auto request=Request(length,bank,offset);
  assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==B_OK
   && request->result.state==TT_RUNTIME_PASSED && c.q38QgwdReceipt->Spans()==1);
  p=D::Packet{};p.operation=D::Seal;assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==B_NOT_ALLOWED);
  FillRemainingSynthetic(c);tt_q38_span_end end={1,0,0,0};
  assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_OK && end.closed);
  assert(QgwdControl(&cookie,&p,sizeof(p))==B_OK && p.accepted && p.sealed && p.spans==21907);
  assert(c.q38QgwdReceipt->Revalidate(&c,&cookie,c.hashes,c.initialized,17,sReloadGeneration));
  auto logical=std::make_unique<PioneerQ38CanonicalModelPlan>();
  assert(PioneerQ38QgwdNativeBinding::Matches(c,&cookie,sReloadGeneration)
   && PioneerQ38QgwdNativeBinding::DeriveLogical(c,&cookie,sReloadGeneration,*logical)
   && logical->Qgw8InspectionLayout() && Q38TokenNativeLearnedPresent(c,*logical));
  auto cohort=std::make_unique<PioneerQ38DispatchLease>();JournalProbe journal;
  assert(!PioneerQ38QgwdNativeBinding::BindJournal(c,&cookie,sReloadGeneration,
   static_cast<PioneerQ38DispatchLease*>(nullptr),journal) && !journal.called);
  assert(!PioneerQ38QgwdNativeBinding::BindJournal(c,&foreign,sReloadGeneration,cohort.get(),journal) && !journal.called);
  assert(PioneerQ38QgwdNativeBinding::BindJournal(c,&cookie,sReloadGeneration,cohort.get(),journal) && journal.called);
  assert(!PioneerQ38QgwdNativeBinding::Matches(c,&foreign,sReloadGeneration)
   && !PioneerQ38QgwdNativeBinding::Matches(c,&cookie,sReloadGeneration+1));
  c.q38Qgw8Receipt=new PioneerQ38Qgw8UploadReceipt;
  assert(!PioneerQ38QgwdNativeBinding::Matches(c,&cookie,sReloadGeneration)
   && !Q38TokenNativeLearnedPresent(c,*logical));
  delete c.q38Qgw8Receipt;c.q38Qgw8Receipt=nullptr;
  c.q38HybridReceipt=new PioneerQ38HybridUploadReceipt;
  assert(!PioneerQ38QgwdNativeBinding::Matches(c,&cookie,sReloadGeneration));
  delete c.q38HybridReceipt;c.q38HybridReceipt=nullptr;
  auto* held=c.q38Span;PioneerQ38SpanLease substituted=*held;c.q38Span=&substituted;
  assert(!PioneerQ38QgwdNativeBinding::Matches(c,&cookie,sReloadGeneration));c.q38Span=held;
  c.hashes[0]^=1;auto corrupt=std::make_unique<PioneerQ38CanonicalModelPlan>();
  assert(!Q38TokenNativeLearnedPresent(c,*logical)
   && !PioneerQ38QgwdNativeBinding::DeriveLogical(c,&cookie,sReloadGeneration,*corrupt)
   && c.banks.Quarantined() && !corrupt->Ready());
  Unbind();
 }
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);D::Packet p;p.operation=D::Begin;copyCalls=0;failCopy=2;
  assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==-99 && c.q38QgwdReceipt
   && c.banks.Quarantined() && c.dispatchOwnerRetained && sColdRecoveryRequired);Unbind();
 }
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);D::Packet p;p.operation=D::Begin;
  assert(QgwdControl(&cookie,&p,sizeof(p))==B_OK);
  unsigned bank=0,length=0;uint64_t offset=0;assert(c.q38QgwdReceipt->Next(bank,offset,length));
  auto request=Request(length,bank,offset);copyCalls=0;failCopy=2;
  assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==-99
   && c.q38Span && c.q38Span->unsafe && c.banks.Quarantined() && c.dispatchOwnerRetained && sColdRecoveryRequired);
  Unbind();
 }
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);D::Packet p;p.operation=D::Begin;
  assert(QgwdControl(&cookie,&p,sizeof(p))==B_OK);
  unsigned bank=0,length=0;uint64_t offset=0;assert(c.q38QgwdReceipt->Next(bank,offset,length));
  auto request=Request(length,bank,offset);
  assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==B_OK);
  tt_q38_span_end end={1,0,0,0};copyCalls=0;failCopy=2;
  assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==-99
   && c.q38Span->unsafe && c.banks.Quarantined() && c.dispatchOwnerRetained && sColdRecoveryRequired);
  Unbind();
 }
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);D::Packet p;p.operation=D::Begin;
  assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==B_OK);
  c.q38Span=new PioneerQ38SpanLease;FillRemainingSynthetic(c);
  c.q38Span->ready=c.q38Span->dirty=false;c.q38Span->closed=true;
  p=D::Packet{};p.operation=D::Seal;copyCalls=0;failCopy=2;
  assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==-99 && c.banks.Quarantined()
   && c.dispatchOwnerRetained && sColdRecoveryRequired && !c.q38QgwdReceipt->Sealed());Unbind();
 }
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);D::Packet p;p.operation=D::Begin;
  assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==B_OK);
  c.q38Span=new PioneerQ38SpanLease;FillRemainingSynthetic(c);
  c.q38Span->ready=c.q38Span->dirty=false;c.q38Span->closed=true;
  sReloadGeneration++;p=D::Packet{};p.operation=D::Seal;
  assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==B_DEV_NOT_READY && c.banks.Quarantined()
   && c.dispatchOwnerRetained && sColdRecoveryRequired && !c.q38QgwdReceipt->Sealed());
  sReloadGeneration--;Unbind();
 }
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);D::Packet p;p.operation=D::Begin;
  assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==B_OK);
  c.q38Span=new PioneerQ38SpanLease;FillRemainingSynthetic(c);
  c.q38Span->ready=c.q38Span->dirty=false;c.q38Span->closed=true;
  auto model=std::make_unique<PioneerQ38CanonicalModelPlan>();auto parent=std::make_unique<PioneerQ38Qgw8KernelLayout>();
  auto layout=std::make_unique<PioneerQ38DenseQ8KernelLayoutInspection>();
  assert(model->Build() && parent->Build(*model) && layout->Build(*model,*parent));unsigned gap=0;
  for(unsigned i=0;i<model->Count();i++)if(layout->At(i)->encoding==PioneerQ38DenseQ8KernelLayoutInspection::DenseQ8Bfp8
   && layout->At(i)->allocated<parent->At(i)->allocated){const auto* q=layout->At(i);
   gap=q->bank*unsigned(PioneerResidentBanks::Capacity/4096)+(q->offset+q->allocated)/4096;break;}
  assert(gap && !c.initialized[gap] && !c.hashes[gap]);c.initialized[gap]=1;c.hashes[gap]=42;
  p=D::Packet{};p.operation=D::Seal;
  assert(Q38QgwdUploadControlInspection(&cookie,&p,sizeof(p))==B_DEV_NOT_READY && c.banks.Quarantined()
   && c.dispatchOwnerRetained && sColdRecoveryRequired && !c.q38QgwdReceipt->Sealed());Unbind();
 }
 assert(acquired==released);
 puts("PASS QGWD control, real span core and sealed gapped receipt; ownership/error checks; SIMULATION ONLY");
}
