/* SPDX-License-Identifier: MIT */
// REAL native debug client -> actual ioctl/ownership -> actual RV32/SFPU.
#define PIONEER_Q38_DELTA_CONTROL_MOCK_ONLY
#include "pioneer_q38_delta_control_test.cpp"
#undef B_DEVICE_OP_CODES_END
#include <Drivers.h>
#define B_DEVICE_OP_CODES_END 1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
static const char* clientImage;
static PioneerProbeCookie* clientCookie=nullptr;
static unsigned clientFault=0,clientOpens=0,clientCloses=0,clientRuns=0,clientAccepts=0,clientEnds=0,clientBegins=0;
static bool clientFailed=false;
static unsigned failedReads=0,failedWrites=0,failedResets=0;
static void MarkClientFailure() {
 assert(clientCookie && clientCookie->context); clientFailed=true;
 auto& io=clientCookie->context->io; failedReads=io.reads; failedWrites=io.writes; failedResets=io.resetWrites;
}
static int DeltaMockOpen(const char* path,int flags) {
 assert(!clientCookie && !strcmp(path,"/dev/misc/tenstorrent_pci_probe") && flags==(O_RDWR|O_CLOEXEC));
 clientOpens++; clientCookie=new PioneerProbeCookie; return 73;
}
static int DeltaMockClose(int fd) {
 assert(fd==73 && clientCookie); clientCloses++;
 if (clientFailed) { auto& io=clientCookie->context->io; assert(io.reads==failedReads && io.writes==failedWrites && io.resetWrites==failedResets); }
 bool retain=clientCookie->context!=nullptr;
 assert(ReleaseResidentCookie(clientCookie)==B_OK);
 if (!retain) clientCookie=nullptr;
 else {
  auto& io=clientCookie->context->io;
  assert(clientFailed && io.reads==failedReads && io.writes==failedWrites && io.resetWrites==failedResets);
 }
 return 0;
}
static int DeltaMockIoctl(int fd,unsigned long op,void* data,size_t bytes) {
 assert(fd==73 && clientCookie && !clientFailed);
 if (op==TT_PCI_PROBE_Q38_BANK_BEGIN) {
  assert(bytes==sizeof(tt_q38_bank_session) && !clientCookie->context && !sRuntimeOwner); clientBegins++;
  clientCookie->context=new PioneerStoreContext(clientImage); sRuntimeOwner=clientCookie; sBankStorageOnly=true;
  auto& r=static_cast<tt_q38_bank_session*>(data)->result; r={}; r.version=1;
  r.state=TT_RESIDENT_SESSION_READY; r.active=r.host_ready=1; r.bytes=PioneerResidentBanks::Total; return 0;
 }
 if (op==TT_PCI_PROBE_Q38_BANK_END) {
  clientEnds++;
  if (clientFault==10) { MarkClientFailure(); return -1; }
  auto& r=static_cast<tt_q38_bank_session*>(data)->result; EndResidentSession(clientCookie,r,true);
  assert(!clientCookie->context); r={}; r.version=1; r.state=TT_RESIDENT_SESSION_CLOSED; r.pci_restored=r.host_released=1; return 0;
 }
 if (op==TT_PCI_PROBE_Q38_DELTA_ACCEPT) {
  clientAccepts++;
  if (clientFault==9) { MarkClientFailure(); return -1; }
 }
 if (op==TT_PCI_PROBE_Q38_DELTA_RUN) {
  clientRuns++; if (clientFault==7) clientCookie->context->io.fault=15;
 }
 int status=Q38DeltaControl(clientCookie,uint32_t(op),data,bytes);
 if (op==TT_PCI_PROBE_Q38_DELTA_SUPPORT && clientFault==8) static_cast<tt_q38_delta_support*>(data)->accept_required=0;
 if (op==TT_PCI_PROBE_Q38_DELTA_SUPPORT && clientFault==11) static_cast<tt_q38_delta_support*>(data)->version=1;
 if (op==TT_PCI_PROBE_Q38_DELTA_SUPPORT && clientFault==12) static_cast<tt_q38_delta_support*>(data)->version=TT_Q38_DELTA_PROBE_VERSION==3?2:3;
 if (op==TT_PCI_PROBE_Q38_DELTA_RUN && clientFault && clientFault<=7) {
  auto& r=*static_cast<tt_q38_delta_probe*>(data);
  if (clientFault==1) r.next[8191]^=1;
  if (clientFault==2) r.result.marker^=1;
  if (clientFault==3) status=-1;
  if (clientFault==4) r.result.after[7]^=1;
  if (clientFault==5) r.prior[8191]^=1;
  if (clientFault==6) r.result.nonce++;
  MarkClientFailure();
 }
 return status==B_OK?0:-1;
}
#define main DeltaClientMain
#define open DeltaMockOpen
#define close DeltaMockClose
#define ioctl DeltaMockIoctl
#include "../pioneer_q38_delta_probe.cpp"
#undef main
#undef open
#undef close
#undef ioctl
int main(int argc,char** argv)
{
 assert(argc==2); clientImage=argv[1]; (void)&Begin;
 (void)&Input; (void)&Prepare; (void)&Upload; (void)&Close; (void)&PreparePanel; (void)&Run; (void)&Job; (void)&ClosePanel;
 char name[]="delta",mode[]="--synthetic"; char* args[]={name,mode,nullptr};
 for (unsigned fault=0;fault<=12;fault++) {
  clientFault=fault; clientOpens=clientCloses=clientRuns=clientAccepts=clientEnds=clientBegins=0; clientFailed=false;
  assert(DeltaClientMain(2,args)==(fault?1:0) && clientOpens==1 && clientCloses==1);
  if (!fault) assert(!clientCookie && clientRuns==PioneerQ38DeltaDebugTrials && clientAccepts==PioneerQ38DeltaDebugTrials && clientEnds==1 && clientBegins==1);
  else if (fault==8 || fault==11 || fault==12) assert(!clientCookie && !clientBegins && !clientRuns && !clientAccepts && !clientEnds);
  else {
   assert(clientFailed && clientCookie && clientBegins==1);
   if (fault<=7) assert(clientRuns==1 && !clientAccepts && !clientEnds);
   if (fault==9) assert(clientRuns==1 && clientAccepts==1 && !clientEnds);
   if (fault==10) assert(clientRuns==PioneerQ38DeltaDebugTrials && clientAccepts==PioneerQ38DeltaDebugTrials && clientEnds==1);
   DiscardRetained(clientCookie); clientCookie=nullptr;
  }
 }
 assert(acquired==released);
 printf("PASS OFFLINE REAL debug client+REAL ioctl+REAL RV32/SFPU:ABI%u %u exact full exports,12 refusal/mismatch/transport faults including opposite scaling ABI;NO ACCEPT/END/retry/further card I/O after failure;ALL owners retained\n",TT_Q38_DELTA_PROBE_VERSION,PioneerQ38DeltaDebugTrials);
}
