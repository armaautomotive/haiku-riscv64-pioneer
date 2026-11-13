/* SPDX-License-Identifier: MIT */
// Actual control and span core with fake OS/device; never a physical test.
#define PIONEER_Q38_SMALL_RECURRENT_MOCK_ROUTE 1
#define main small_span_unused_main
#include "pioneer_q38_small_recurrent_span_test.cpp"
#undef main
static int small_geteuid(){return 0;}
#define geteuid small_geteuid
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SmallRecurrentUploadControl.inc"
#undef geteuid
namespace H=PioneerQ38SmallRecurrentUpload;
using Packet=H::Packet;
static void FillRemaining(NativeSpanContext& c){
 unsigned bank=0,length=0;uint64_t offset=0;
 while(c.q38SmallRecurrentReceipt->Next(bank,offset,length)){
  assert(c.q38SmallRecurrentReceipt->Before(&c,c.hashes,c.initialized,c.q38Span,bank,offset,length));
  const unsigned first=bank*unsigned(PioneerResidentBanks::Capacity/4096)+unsigned(offset/4096);
  for(unsigned i=0;i<length/4096;i++){
   const unsigned p=first+i;const uint32_t hash=p*2654435761U;c.initialized[p]=1;c.hashes[p]=hash;
   assert(c.q38SmallRecurrentReceipt->RecordPage(p,hash));
  }
  c.q38Span->saved=c.q38Span->ready=c.q38Span->dirty=true;c.q38Span->nonce++;
  assert(c.q38SmallRecurrentReceipt->After());
 }
 assert(c.q38SmallRecurrentReceipt->Spans()==21475);
}
int main(int argc,char** argv){
 assert(argc==2);PioneerProbeCookie cookie,foreign;
 for(unsigned lost=0;lost<2;lost++){
  NativeSpanContext c(argv[1]);Bind(cookie,c);Packet p;p.operation=H::Begin;
  Packet malformed=p;malformed.magic^=1;
  assert(Q38SmallRecurrentUploadControl(&cookie,&malformed,sizeof(malformed))==B_BAD_VALUE);
  team=18;assert(Q38SmallRecurrentUploadControl(&cookie,&p,sizeof(p))==B_NOT_ALLOWED);team=17;
  assert(Q38SmallRecurrentUploadControl(&foreign,&p,sizeof(p))==B_BUSY);
  c.q38QgwdReceipt=&c;assert(Q38SmallRecurrentUploadControl(&cookie,&p,sizeof(p))==B_NOT_ALLOWED);c.q38QgwdReceipt=nullptr;
  c.q38RecurrentReceipt=reinterpret_cast<PioneerQ38RecurrentQ8UploadReceiptInspection*>(&c);
  assert(Q38SmallRecurrentUploadControl(&cookie,&p,sizeof(p))==B_NOT_ALLOWED);c.q38RecurrentReceipt=nullptr;
  assert(Q38SmallRecurrentUploadControl(&cookie,&p,sizeof(p))==B_OK && p.accepted && !p.sealed && !p.spans);
  Packet duplicate;duplicate.operation=H::Begin;assert(Q38SmallRecurrentUploadControl(&cookie,&duplicate,sizeof(duplicate))==B_NOT_ALLOWED);
  c.q38Span=new PioneerQ38SpanLease;unsigned bank=0,length=0;uint64_t offset=0;
  assert(c.q38SmallRecurrentReceipt->Next(bank,offset,length));auto request=Request(length,bank,offset);
  assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==B_OK
   && request->result.state==TT_RUNTIME_PASSED && c.q38SmallRecurrentReceipt->Spans()==1);
  Packet seal;seal.operation=H::Seal;assert(Q38SmallRecurrentUploadControl(&cookie,&seal,sizeof(seal))==B_NOT_ALLOWED);
  FillRemaining(c);tt_q38_span_end end={1,0,0,0};
  assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_OK && end.closed);
  if(lost){
   failCopy=copyCalls+2;
   assert(Q38SmallRecurrentUploadControl(&cookie,&seal,sizeof(seal))==-99 && c.banks.Quarantined()
    && c.dispatchOwnerRetained && sColdRecoveryRequired && !c.q38SmallRecurrentReceipt->Sealed());
  }else{
   assert(Q38SmallRecurrentUploadControl(&cookie,&seal,sizeof(seal))==B_OK && seal.accepted && seal.sealed && seal.spans==21475);
   assert(c.q38SmallRecurrentReceipt->Revalidate(&c,&cookie,c.hashes,c.initialized,17,sReloadGeneration));
  }
  Unbind();
 }
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);Packet p;p.operation=H::Begin;copyCalls=0;failCopy=2;
  assert(Q38SmallRecurrentUploadControl(&cookie,&p,sizeof(p))==-99 && c.q38SmallRecurrentReceipt
   && c.banks.Quarantined() && c.Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired);
  const unsigned touches=c.io.touches;failCopy=0;Packet retry;retry.operation=H::Begin;
  assert(Q38SmallRecurrentUploadControl(&cookie,&retry,sizeof(retry))==B_DEV_NOT_READY && c.io.touches==touches);Unbind();
 }
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);Packet p;p.operation=H::Begin;
  assert(Q38SmallRecurrentUploadControl(&cookie,&p,sizeof(p))==B_OK);
  c.q38Span=new PioneerQ38SpanLease;unsigned bank=0,length=0;uint64_t offset=0;
  assert(c.q38SmallRecurrentReceipt->Next(bank,offset,length));auto request=Request(length,bank,offset);
  failCopy=copyCalls+2;
  assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==-99
   && c.banks.Quarantined() && c.Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired);
  failCopy=0;Unbind();
 }
 puts("PASS small recurrent upload control, span core and lost-reply quarantine; SIMULATED NO DEVICE");
}
