/* SPDX-License-Identifier: MIT */
// Real span core and distinct inspection control on simulated OS and card.
#define PIONEER_Q38_RECURRENT_Q8_CONTROL_INSPECTION 1
#define PIONEER_Q38_RECURRENT_Q8_UPLOAD_INSPECTION 1
#include "pioneer_q38_recurrent_q8_span_test.cpp"
static int recurrent_geteuid(){return 0;}
#define geteuid recurrent_geteuid
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentQ8UploadControlInspection.inc"
#undef geteuid
using Packet=PioneerQ38RecurrentQ8UploadInspection::Packet;
namespace H=PioneerQ38RecurrentQ8UploadInspection;
static void FillRemaining(NativeSpanContext& c){
 unsigned bank=0,length=0;uint64_t offset=0;
 while(c.q38RecurrentReceipt->Next(bank,offset,length)){
  assert(c.q38RecurrentReceipt->Before(&c,c.hashes,c.initialized,c.q38Span,bank,offset,length));
  const unsigned first=bank*unsigned(PioneerResidentBanks::Capacity/4096)+unsigned(offset/4096);
  for(unsigned i=0;i<length/4096;i++){
   const unsigned p=first+i;const uint32_t hash=p*2654435761U;c.initialized[p]=1;c.hashes[p]=hash;
   assert(c.q38RecurrentReceipt->RecordPage(p,hash));
  }
  c.q38Span->saved=c.q38Span->ready=c.q38Span->dirty=true;c.q38Span->nonce++;
  assert(c.q38RecurrentReceipt->After());
 }
 assert(c.q38RecurrentReceipt->Spans()==21475);
}
int main(int argc,char** argv){
 assert(argc==2);PioneerProbeCookie cookie,foreign;
 for(unsigned lost=0;lost<2;lost++){
  NativeSpanContext c(argv[1]);Bind(cookie,c);Packet p;p.operation=H::Begin;
  Packet malformed=p;malformed.magic^=1;
  assert(Q38RecurrentUploadControlInspection(&cookie,&malformed,sizeof(malformed))==B_BAD_VALUE);
  team=18;assert(Q38RecurrentUploadControlInspection(&cookie,&p,sizeof(p))==B_NOT_ALLOWED);team=17;
  assert(Q38RecurrentUploadControlInspection(&foreign,&p,sizeof(p))==B_BUSY);
  c.q38Qgw8Receipt=&c;assert(Q38RecurrentUploadControlInspection(&cookie,&p,sizeof(p))==B_NOT_ALLOWED);c.q38Qgw8Receipt=nullptr;
  c.q38QgwdReceipt=&c;assert(Q38RecurrentUploadControlInspection(&cookie,&p,sizeof(p))==B_NOT_ALLOWED);c.q38QgwdReceipt=nullptr;
  c.q38HybridReceipt=new PioneerQ38HybridUploadReceipt;
  assert(Q38RecurrentUploadControlInspection(&cookie,&p,sizeof(p))==B_NOT_ALLOWED);
  delete c.q38HybridReceipt;c.q38HybridReceipt=nullptr;
  failCopy=copyCalls+1;assert(Q38RecurrentUploadControlInspection(&cookie,&p,sizeof(p))==-99
   && !c.q38RecurrentReceipt && !c.q38Span && !c.Unsafe());failCopy=0;
  assert(Q38RecurrentUploadControlInspection(&cookie,&p,sizeof(p))==B_OK && p.accepted && !p.sealed && !p.spans);
  Packet duplicate;duplicate.operation=H::Begin;assert(Q38RecurrentUploadControlInspection(&cookie,&duplicate,sizeof(duplicate))==B_NOT_ALLOWED);
  c.q38Span=new PioneerQ38SpanLease;unsigned bank=0,length=0;uint64_t offset=0;
  assert(c.q38RecurrentReceipt->Next(bank,offset,length));auto request=Request(length,bank,offset);
  assert(PioneerQ38RecurrentQ8SpanUploadFastInspection(c,*request,c.q38RecurrentReceipt)
   && request->result.state==TT_RUNTIME_PASSED && c.q38RecurrentReceipt->Spans()==1);
  Packet seal;seal.operation=H::Seal;assert(Q38RecurrentUploadControlInspection(&cookie,&seal,sizeof(seal))==B_NOT_ALLOWED);
  FillRemaining(c);
  // The legacy ioctl deliberately does not dispatch this new receipt; close
  // the real span service directly in this offline fixture, never via ioctl.
  assert(PioneerQ38SpanEndService(c) && c.q38Span->closed);
  if(lost){
   failCopy=copyCalls+2;
   assert(Q38RecurrentUploadControlInspection(&cookie,&seal,sizeof(seal))==-99 && c.banks.Quarantined()
    && c.dispatchOwnerRetained && sColdRecoveryRequired && !c.q38RecurrentReceipt->Sealed());
   const unsigned touches=c.io.touches;failCopy=0;Packet retry;retry.operation=H::Seal;
   assert(Q38RecurrentUploadControlInspection(&cookie,&retry,sizeof(retry))==B_DEV_NOT_READY && c.io.touches==touches);
  }else{
   assert(Q38RecurrentUploadControlInspection(&cookie,&seal,sizeof(seal))==B_OK && seal.accepted && seal.sealed && seal.spans==21475);
   assert(c.q38RecurrentReceipt->Revalidate(&c,&cookie,c.hashes,c.initialized,17,sReloadGeneration));
  }
  Unbind();
 }
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);Packet p;p.operation=H::Begin;copyCalls=0;failCopy=2;
  assert(Q38RecurrentUploadControlInspection(&cookie,&p,sizeof(p))==-99 && c.q38RecurrentReceipt
   && c.banks.Quarantined() && c.Unsafe() && c.dispatchOwnerRetained && sColdRecoveryRequired);
  const unsigned touches=c.io.touches;failCopy=0;Packet retry;retry.operation=H::Begin;
  assert(Q38RecurrentUploadControlInspection(&cookie,&retry,sizeof(retry))==B_DEV_NOT_READY && c.io.touches==touches);Unbind();
 }
 puts("PASS recurrent control, real span core, synthetic full receipt and lost-reply quarantine; NO DEVICE");
}
