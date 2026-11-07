/* SPDX-License-Identifier: MIT */
// Separate QGW8 inspection control + actual service; simulated OS/device.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_QGW8_SPAN_INSPECTION 1
#define PIONEER_Q38_QGW8_CONTROL_INSPECTION 1
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_SPAN_FAST 1
#include "pioneer_q38_span_service_test.cpp"
static int qgw8_fixture_geteuid(){return 0;}
#define geteuid qgw8_fixture_geteuid
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8UploadControlInspection.inc"
#undef geteuid

static void CompleteSyntheticReceipt(NativeSpanContext& c){
 assert(c.q38Qgw8Receipt && !c.q38Span);c.q38Span=new PioneerQ38SpanLease;
 unsigned bank=0,length=0,spans=0;uint64_t offset=0;
 while(c.q38Qgw8Receipt->Next(bank,offset,length)){
  assert(c.q38Qgw8Receipt->Before(&c,c.hashes,c.initialized,c.q38Span,bank,offset,length));
  const unsigned first=bank*unsigned(PioneerResidentBanks::Capacity/4096)+unsigned(offset/4096);
  for(unsigned i=0;i<length/4096;i++){
   const uint32_t hash=(first+i)*2654435761U;c.initialized[first+i]=1;c.hashes[first+i]=hash;
   assert(c.q38Qgw8Receipt->RecordPage(first+i,hash));
  }
  c.q38Span->saved=c.q38Span->ready=c.q38Span->dirty=true;c.q38Span->nonce=++spans;
  assert(c.q38Qgw8Receipt->After());
 }
 assert(spans==22035);c.q38Span->ready=c.q38Span->dirty=false;c.q38Span->closed=true;
}
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;
 (void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 namespace Q=PioneerQ38Qgw8Upload;namespace H=PioneerQ38HybridUpload;
 for(unsigned i=0;i<8;i++){
  Q::Packet bad;bad.operation=Q::Begin;
  uint32_t* fields[]={&bad.magic,&bad.version,&bad.operation,&bad.reserved,&bad.accepted,&bad.sealed,&bad.spans,&bad.recovery_required};
  *fields[i]^=1;assert(!Q::Request(bad));
 }
 PioneerProbeCookie cookie,foreign;
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);Q::Packet packet;packet.operation=Q::Begin;
  H::Packet legacy;std::memcpy(&legacy,&packet,sizeof(legacy));assert(!H::Request(legacy) && Q::Request(packet));
  packet.magic=H::Magic;assert(Q38Qgw8UploadControlInspection(&cookie,&packet,sizeof(packet))==B_BAD_VALUE && !c.q38Qgw8Receipt);
  packet.magic=Q::Magic;packet.reserved=1;
  assert(Q38Qgw8UploadControlInspection(&cookie,&packet,sizeof(packet))==B_BAD_VALUE && !c.q38Qgw8Receipt);
  packet.reserved=0;team=18;assert(Q38Qgw8UploadControlInspection(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED);team=17;
  assert(Q38Qgw8UploadControlInspection(&foreign,&packet,sizeof(packet))==B_BUSY && !c.q38Qgw8Receipt);
  c.q38HybridReceipt=new PioneerQ38HybridUploadReceipt;
  assert(Q38Qgw8UploadControlInspection(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED && !c.q38Qgw8Receipt);
  delete c.q38HybridReceipt;c.q38HybridReceipt=nullptr;
  assert(Q38Qgw8UploadControlInspection(&cookie,&packet,sizeof(packet))==B_OK && packet.accepted && !packet.sealed && !packet.spans);
  H::Packet oldPacket;oldPacket.operation=H::Begin;
  assert(Q38HybridUploadControl(&cookie,&oldPacket,sizeof(oldPacket))==B_NOT_ALLOWED && !c.q38HybridReceipt);
  packet=Q::Packet{};packet.operation=Q::Begin;
  assert(Q38Qgw8UploadControlInspection(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED);
  unsigned bank=0,length=0;uint64_t offset=0;assert(c.q38Qgw8Receipt->Next(bank,offset,length));
  c.q38Span=new PioneerQ38SpanLease;auto request=Request(length,bank,offset);
  assert(PioneerQ38Qgw8SpanUploadInspection(c,*request,c.q38Qgw8Receipt)
   && request->result.state==TT_RUNTIME_PASSED && c.q38Qgw8Receipt->Spans()==1);
  packet=Q::Packet{};packet.operation=Q::Seal;
  assert(Q38Qgw8UploadControlInspection(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED && !c.Unsafe());
  assert(PioneerQ38SpanEndService(c));
  assert(Q38Qgw8UploadControlInspection(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED && !c.Unsafe());
  Unbind();
 }
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);Q::Packet packet;packet.operation=Q::Begin;
  copyCalls=0;failCopy=2;assert(Q38Qgw8UploadControlInspection(&cookie,&packet,sizeof(packet))==-99);
  assert(c.q38Qgw8Receipt && c.banks.Quarantined() && c.dispatchOwnerRetained && sColdRecoveryRequired);
  Unbind();
 }
 for(unsigned fault=0;fault<2;fault++){
  NativeSpanContext c(argv[1]);Bind(cookie,c);Q::Packet packet;packet.operation=Q::Begin;
  assert(Q38Qgw8UploadControlInspection(&cookie,&packet,sizeof(packet))==B_OK);
  CompleteSyntheticReceipt(c);packet=Q::Packet{};packet.operation=Q::Seal;copyCalls=0;
  if(fault)failCopy=2;
  const status_t status=Q38Qgw8UploadControlInspection(&cookie,&packet,sizeof(packet));
  if(fault){
   assert(status==-99 && c.banks.Quarantined() && c.dispatchOwnerRetained && sColdRecoveryRequired
    && !c.q38Qgw8Receipt->Sealed());
  }else assert(status==B_OK && packet.accepted && packet.sealed && packet.spans==22035
   && c.q38Qgw8Receipt->Sealed());
  Unbind();
 }
 assert(acquired==released);
 puts("PASS QGW8 inspection control: distinct packet, team/owner/legacy exclusion, one simulated span, full synthetic ledger seal and reply-fault quarantine; NO CARD");
}
