/* SPDX-License-Identifier: MIT */
// Production control and span handler with simulated OS/device endpoints.
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_SPAN_FAST 1
#include "pioneer_q38_span_service_test.cpp"
int main(int argc,char** argv){
 assert(argc==2);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 namespace H=PioneerQ38HybridUpload;PioneerProbeCookie cookie,foreign;
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);auto request=Request();unsigned touches=c.io.touches;
  assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==B_NOT_ALLOWED && c.io.touches==touches);
  H::Packet packet;packet.operation=H::Begin;packet.reserved=1;
  assert(Q38HybridUploadControl(&cookie,&packet,sizeof(packet))==B_BAD_VALUE && !c.q38HybridReceipt);
  packet.reserved=0;team=18;assert(Q38HybridUploadControl(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED);team=17;
  assert(Q38HybridUploadControl(&foreign,&packet,sizeof(packet))==B_BUSY && !c.q38HybridReceipt);
  assert(Q38HybridUploadControl(&cookie,&packet,sizeof(packet))==B_OK && packet.accepted && !packet.sealed && !packet.spans);
  packet=H::Packet{};packet.operation=H::Begin;assert(Q38HybridUploadControl(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED);
  unsigned bank=0,length=0;uint64_t offset=0;assert(c.q38HybridReceipt->Next(bank,offset,length));
  request=Request(length,bank,offset);
  assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request))==B_OK
   && request->result.state==TT_RUNTIME_PASSED && c.q38HybridReceipt->Spans()==1);
  packet=H::Packet{};packet.operation=H::Seal;
  assert(Q38HybridUploadControl(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED && !c.Unsafe());
  tt_q38_span_end end={1,0,0,0};assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==B_OK && end.closed);
  assert(Q38HybridUploadControl(&cookie,&packet,sizeof(packet))==B_NOT_ALLOWED && !c.Unsafe());
  Unbind();
 }
 {
  NativeSpanContext c(argv[1]);Bind(cookie,c);H::Packet packet;packet.operation=H::Begin;
  copyCalls=0;failCopy=2;assert(Q38HybridUploadControl(&cookie,&packet,sizeof(packet))==-99);
  assert(c.q38HybridReceipt && c.banks.Quarantined() && c.dispatchOwnerRetained && sColdRecoveryRequired);
  Unbind();
 }
 for(unsigned fault=0;fault<2;fault++){
  NativeSpanContext c(argv[1]);Bind(cookie,c);H::Packet packet;packet.operation=H::Begin;
  assert(Q38HybridUploadControl(&cookie,&packet,sizeof(packet))==B_OK);
  unsigned bank=0,length=0;uint64_t offset=0;assert(c.q38HybridReceipt->Next(bank,offset,length));
  auto request=Request(length,bank,offset);copyCalls=0;if(!fault)failCopy=2;
  const status_t status=Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD,request.get(),sizeof(*request));
  if(fault){
   assert(status==B_OK && request->result.state==TT_RUNTIME_PASSED);
   tt_q38_span_end end={1,0,0,0};copyCalls=0;failCopy=2;
   assert(Q38SpanControl(&cookie,TT_PCI_PROBE_Q38_SPAN_END,&end,sizeof(end))==-99);
  }else assert(status==-99);
  assert(c.banks.Quarantined() && c.q38Span->unsafe && c.dispatchOwnerRetained && sColdRecoveryRequired);
  Unbind();
 }
 assert(acquired==released);puts("PASS real tracked upload control and span handler: same FD/team/lock, begin, ordered transfer, incomplete seal refusal and reply-failure quarantine; SIMULATED DEVICE ONLY");
}
