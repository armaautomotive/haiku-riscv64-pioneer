/* SPDX-License-Identifier: MIT */
// Real span core and recurrent ledger, with simulated OS and card only.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_KERNEL_LAYOUT_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_CANONICAL_INSPECTION 1
#define PIONEER_Q38_DENSE_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_RECURRENT_Q8_SPAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_TRACKED_UPLOAD 1
#define PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION 1
#define PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION 1
#define PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_Q38_SPAN_FAST 1
#define PIONEER_BANKS_MOCK_ONLY
#include "../PioneerQ38RecurrentQ8MatrixWorkspace.h"
#include "pioneer_q38_span_service_test.cpp"

#ifndef PIONEER_Q38_RECURRENT_Q8_CONTROL_INSPECTION
int main(int argc,char** argv) {
 assert(argc==2);
 for(unsigned mode=0;mode<3;mode++) {
  PioneerProbeCookie cookie; NativeSpanContext c(argv[1]); Bind(cookie,c);
  c.q38Span=new PioneerQ38SpanLease;
  auto receipt=std::make_unique<PioneerQ38RecurrentQ8UploadReceiptInspection>();
  assert(receipt->Begin(&c,&cookie,c.hashes,c.initialized,cookie.team,sReloadGeneration,
   c.banks,c.baseline.baseline.baseline.baseline.dram,unsigned(c.bytes/4096)));
  unsigned bank=0,length=0;uint64_t offset=0;assert(receipt->Next(bank,offset,length));
  auto request=Request(length,bank,offset);
  if(mode==2)c.io.packetFault=1;
  const bool ok=mode==0?PioneerQ38RecurrentQ8SpanUploadInspection(c,*request,receipt.get())
   :PioneerQ38RecurrentQ8SpanUploadFastInspection(c,*request,receipt.get());
  if(mode<2) {
   assert(ok && request->result.state==TT_RUNTIME_PASSED && receipt->Spans()==1
    && c.uploadedPages==length/4096 && !c.Unsafe());
   const unsigned touches=c.io.touches;
   assert(!PioneerQ38RecurrentQ8SpanUploadFastInspection(c,*request,receipt.get())
    && c.io.touches==touches && receipt->Spans()==1);
  } else assert(!ok && c.banks.Quarantined() && receipt->Spans()==0);
  Unbind();
 }
}
#endif
