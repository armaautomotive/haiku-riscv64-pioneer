/* SPDX-License-Identifier: MIT */
// REAL client envelope -> REAL gated handler/lock/bank claims. Span/END
// replies and initializer are EXPLICIT MOCKS,NOT native IO qualification.
#define PIONEER_Q38_GDN_ZERO_CONTROL_MODEL_ONLY
#include "pioneer_q38_gdn_zero_control_test.cpp"
#include "../PioneerQ38GdnZeroDebug.h"
struct ClientIO {
 Fixture f{false}; PioneerQ38SpanLease span;
 unsigned fault=0,opens=0,closes=0,begins=0,ends=0,calls=0,zeros=0;
 bool closed=false,failed=false;
 explicit ClientIO(unsigned which):fault(which) { Ready(f,span); span.closed=false; }
 int Open() { opens++; return 73; }
 bool Close(int fd) {
  assert(fd==73 && !closed); closes++; closed=true;
  if (begins && !ends) { tt_resident_session_result r={}; RetainQ38DispatchOwner(f.c,r); }
  return true;
 }
 bool Call(int fd,uint32_t op,void* data,size_t bytes) {
  assert(fd==73 && !closed && !failed); calls++;
  bool okay=true;
  if (op==TT_PCI_PROBE_Q38_GDN_ZERO_SUPPORT) {
   okay=Q38GdnZeroControl(&f.cookie,op,data,bytes)==B_OK;
   if (fault==1) static_cast<tt_q38_gdn_zero_support*>(data)->version++;
   if (fault==18) okay=false;
  } else if (op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT) {
   assert(bytes==sizeof(tt_q38_span_support)); *static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};
   if (fault==2) static_cast<tt_q38_span_support*>(data)->before_cohort=0;
  } else if (op==TT_PCI_PROBE_Q38_BANK_SUPPORT) {
   assert(bytes==sizeof(tt_q38_bank_support)); *static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,PioneerResidentBanks::Capacity};
   if (fault==3) static_cast<tt_q38_bank_support*>(data)->storage_only=0;
  } else if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) {
   assert(!begins && bytes==sizeof(tt_q38_bank_session)); begins++;
   auto& r=static_cast<tt_q38_bank_session*>(data)->result; r={}; r.version=1; r.state=TT_RESIDENT_SESSION_READY;
   r.active=r.host_ready=1; r.bytes=PioneerResidentBanks::Total;
   if (fault==4) r.state=TT_RESIDENT_SESSION_CLOSED;
   if (fault==21) okay=false;
  } else if (op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD) {
   assert(begins==1 && bytes==sizeof(tt_q38_span_upload)); auto& r=*static_cast<tt_q38_span_upload*>(data);
   assert(r.version==1 && !r.bank && !r.offset && r.bytes==4096 && !r.reserved);
   for (unsigned w=0;w<1024;w++) assert(r.data[w]==(0x5147385aU^(w*0x9e3779b9U)));
   f.initialized[0]=1; r.result={1,TT_RUNTIME_PASSED,4096,1,1,1,1,1,0,0,0,0};
   if (fault==5) r.result.nonce++; if (fault==6) r.result.recovery_required=1;
   if (fault==19) okay=false;
  } else if (op==TT_PCI_PROBE_Q38_SPAN_END) {
   assert(begins==1 && bytes==sizeof(tt_q38_span_end)); auto& r=*static_cast<tt_q38_span_end*>(data); r.closed=1; span.closed=true;
   if (fault==7) r.closed=0; if (fault==20) okay=false;
  } else if (op==TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE) {
   zeros++; assert(zeros==1 && bytes==sizeof(tt_q38_gdn_zero_request));
   if (fault==17) initializeFault=1;
   if (fault==22) failCopy=copyCalls+2; // request copy succeeds,reply copy fails:actual handler retains
   okay=Q38GdnZeroControl(&f.cookie,op,data,bytes)==B_OK;
   auto& r=*static_cast<tt_q38_gdn_zero_request*>(data);
   if (fault==8) okay=false; if (fault==9) r.pages--; if (fault==10) r.initialization_published^=1;
   if (fault==11) r.reserved[1]=1; if (fault==12) r.mode=r.mode==1?2:1; if (fault==13) r.closed=0;
   if (fault==14) r.state=TT_RUNTIME_COMPLETION_FAILED;
   assert(f.initialized[0]==1); // real fresh-bank claims must NEVER consume the uploaded page
   PioneerQ38GdnBankExtent first;
   if (!f.c.retained) assert(f.c.q38GdnBanks->InitializeHalf(f.c.q38GdnIdentity,0,0,0,first)
    && (first.bank || first.address!=PioneerResidentBanks::Base));
  } else {
   assert(op==TT_PCI_PROBE_Q38_BANK_END && begins==1 && zeros==1 && !ends && bytes==sizeof(tt_q38_bank_session)); ends++;
   auto& r=static_cast<tt_q38_bank_session*>(data)->result; r={}; r.version=1; r.state=TT_RESIDENT_SESSION_CLOSED; r.pci_restored=r.host_released=1;
   if (fault==15) r.pci_restored=0; if (fault==16) okay=false;
  }
  if (!okay) failed=true; return okay;
 }
};
int main() {
 (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close;
 for (unsigned mode:{1U,2U}) for (unsigned fault=0;fault<=22;fault++) {
  ClientIO io(fault); bool okay=PioneerQ38GdnZeroDebug(io,mode);
  assert(okay==!fault && io.opens==1 && io.closes==1 && !io.f.c.io.touches && acquired==released);
  if (!fault) assert(io.begins==1 && io.zeros==1 && io.ends==1 && !io.f.c.retained);
  else {
   assert(io.ends==unsigned(fault==15 || fault==16)); // never END after failed zero/span/reply
   if (io.begins && !io.ends) assert(io.f.c.retained && sColdRecoveryRequired);
   if (fault==22) assert(io.f.c.retained && io.f.c.q38GdnBanks && io.f.c.q38StateZero->unsafe);
  }
 }
 { ClientIO io(0); assert(PioneerQ38GdnZeroDebug(io,1,true) && io.calls==1 && io.closes==1 && !io.begins && !io.zeros); }
 puts("PASS OFFLINE REAL fixed zero client+REAL gated control/lock/bank claims:BOTH modes,22 transport/reply/failure controls,CLOSE ONLY after failure,no subsequent calls/END/retry;span/END/initializers EXPLICITLY MOCKED,NOT native/RV32/data-zero qualification");
}
