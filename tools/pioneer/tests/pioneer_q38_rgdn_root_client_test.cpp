/* SPDX-License-Identifier: MIT */
// REAL debug client -> REAL control/guard/controller/RV32/SFPU. Setup/
// BANK_END are EXPLICIT envelope mocks;initial zero ledger counter fabric.
#define PIONEER_Q38_RGDN_ROOT_CONTROL_MODEL_ONLY
#include "pioneer_q38_rgdn_root_control_test.cpp"
#include "../PioneerQ38RgdnRootDebug.h"
struct ClientIO {
 ControlFixture f;
 unsigned fault,opens=0,closes=0,begins=0,runs=0,accepts=0,rootEnds=0,bankEnds=0;
 bool closed=false,failed=false;
 ClientIO(const char* zero,const char* root,unsigned bad):f(zero,root),fault(bad) {}
 int Open() { opens++; return 73; }
 bool Close(int fd) {
  assert(fd==73 && !closed); closes++; closed=true;
  if (begins && !bankEnds) { tt_resident_session_result r={}; RetainQ38DispatchOwner(f.c,r); }
  return true;
 }
 bool Call(int fd,uint32_t op,void* data,size_t bytes) {
  assert(fd==73 && !closed && !failed); bool okay=true;
  if (op>=TT_PCI_PROBE_Q38_RGDN_ROOT_SUPPORT && op<=TT_PCI_PROBE_Q38_RGDN_ROOT_END) {
   if (op==TT_PCI_PROBE_Q38_RGDN_ROOT_RUN) { runs++; if (fault==13) failCopy=copyCalls+2; }
   if (op==TT_PCI_PROBE_Q38_RGDN_ROOT_ACCEPT) { accepts++; if (fault==11) { failed=true; return false; } }
   if (op==TT_PCI_PROBE_Q38_RGDN_ROOT_END) rootEnds++;
   okay=Q38RgdnRootControl(&f.cookie,op,data,bytes)==B_OK;
   if (op==TT_PCI_PROBE_Q38_RGDN_ROOT_SUPPORT) {
    if (fault==1) static_cast<tt_q38_rgdn_root_support*>(data)->version++;
    if (fault==2) static_cast<tt_q38_rgdn_root_support*>(data)->accept_required=0;
   }
   if (op==TT_PCI_PROBE_Q38_RGDN_ROOT_RUN) {
    auto& r=*static_cast<tt_q38_rgdn_root_request*>(data);
    if (fault==3) r.next[8191]^=1; if (fault==4) r.output[63]^=1; if (fault==5) r.query[127]^=1;
    if (fault==6) r.result.nonce++; if (fault==7) r.result.position++; if (fault==8) r.result.read_slot^=1;
    if (fault==9) r.result.recovery_required=1; if (fault==10) r.result.marker^=1;
   }
   if (op==TT_PCI_PROBE_Q38_RGDN_ROOT_END && fault==12) static_cast<tt_q38_rgdn_root_end*>(data)->closed=0;
  } else if (op==TT_PCI_PROBE_Q38_GDN_ZERO_SUPPORT) {
   assert(bytes==sizeof(tt_q38_gdn_zero_support)); *static_cast<tt_q38_gdn_zero_support*>(data)={1,55,1,73728,672,1,0,1};
  } else if (op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT) {
   assert(bytes==sizeof(tt_q38_span_support)); *static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0};
  } else if (op==TT_PCI_PROBE_Q38_BANK_SUPPORT) {
   assert(bytes==sizeof(tt_q38_bank_support)); *static_cast<tt_q38_bank_support*>(data)={45,1,7,4096,4,1,PioneerResidentBanks::Capacity};
  } else if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) {
   assert(!begins && bytes==sizeof(tt_q38_bank_session)); begins++;
   auto& r=static_cast<tt_q38_bank_session*>(data)->result; r={}; r.version=1; r.state=TT_RESIDENT_SESSION_READY;
   r.active=r.host_ready=1; r.bytes=PioneerResidentBanks::Total;
  } else if (op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD) {
   assert(begins==1 && bytes==sizeof(tt_q38_span_upload)); auto& r=*static_cast<tt_q38_span_upload*>(data);
   assert(r.version==1 && !r.bank && !r.offset && r.bytes==4096 && !r.reserved);
   for (unsigned w=0;w<1024;w++) assert(r.data[w]==(0x5147385aU^(w*0x9e3779b9U)));
   r.result={1,TT_RUNTIME_PASSED,4096,1,1,1,1,1,0,0,0,0};
  } else if (op==TT_PCI_PROBE_Q38_SPAN_END) {
   assert(bytes==sizeof(tt_q38_span_end)); static_cast<tt_q38_span_end*>(data)->closed=1;
  } else if (op==TT_PCI_PROBE_Q38_GDN_ZERO_INITIALIZE) {
   assert(bytes==sizeof(tt_q38_gdn_zero_request)); auto& r=*static_cast<tt_q38_gdn_zero_request*>(data);
   assert(r.version==1 && r.mode==2 && !r.reserved[0] && !r.reserved[1]);
   r.state=TT_RUNTIME_PASSED; r.regions=r.jobs=672; r.pages=73728; r.closed=r.initialization_published=1;
   if (fault==14) r.initialization_published=0;
  } else {
   assert(op==TT_PCI_PROBE_Q38_BANK_END && rootEnds==1 && f.c.q38ResidentGdnRoot->closed && bytes==sizeof(tt_q38_bank_session)); bankEnds++;
   auto& r=static_cast<tt_q38_bank_session*>(data)->result; r={}; r.version=1; r.state=TT_RESIDENT_SESSION_CLOSED; r.pci_restored=r.host_released=1;
  }
  if (!okay) failed=true; return okay;
 }
};
int main(int argc,char** argv) {
 assert(argc==3); setvbuf(stdout,nullptr,_IOLBF,0); (void)&TestWorker;
 (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel; (void)&Finish;
 for (unsigned trials:{1U,16U}) {
  ClientIO io(argv[1],argv[2],0); assert(PioneerQ38RgdnRootDebug(io,trials) && io.opens==1 && io.closes==1
   && io.runs==trials && io.accepts==trials && io.rootEnds==1 && io.bankEnds==1 && !io.f.c.Unsafe() && acquired==released);
 }
 for (unsigned fault=1;fault<=14;fault++) {
  ClientIO io(argv[1],argv[2],fault); assert(!PioneerQ38RgdnRootDebug(io,1) && io.opens==1 && io.closes==1 && !io.bankEnds);
  if (fault<=2 || fault==14) assert(!io.runs && !io.accepts && !io.rootEnds);
  else {
   assert(io.runs==1 && io.accepts==unsigned(fault==11 || fault==12) && io.rootEnds==unsigned(fault==12));
   assert(io.f.c.dispatchOwnerRetained && sColdRecoveryRequired && io.f.c.q38ResidentGdnRoot->unsafe && acquired==released);
  }
 }
 puts("PASS OFFLINE REAL resident debug client+actual entry/guard/controller/RV32/SFPU:1/16 exact8256-word state transitions,ACK ONLY after full CPU-debug validation,14 support/input/output/nonce/slot/state/init/ACK/reply fault controls CLOSE ONLY without END/retry;setup/BANK_END mocked,zero counter fabric,NO native or Work-speed claim");
}
