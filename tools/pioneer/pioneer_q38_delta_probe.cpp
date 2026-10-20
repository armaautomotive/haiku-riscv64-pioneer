/* SPDX-License-Identifier: MIT */
// Explicit CPU-checked LOCAL recurrent-state primitive qualification only.
// Not a model runner,Work backend,or speed estimate. No arbitrary inputs.
#include <Drivers.h>
#include <OS.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "PioneerQ38DeltaProbeDebug.h"
static bool DeltaCall(int fd,uint32_t op,void* data,size_t bytes)
{
 if (!ioctl(fd,op,data,bytes)) return true;
 fprintf(stderr,"STOP:LOCAL_DELTA op=%08x errno=%d;NO_RETRY\n",op,errno); return false;
}
static tt_q38_delta_probe deltaRequest,deltaInputs;
int main(int argc,char** argv)
{
 if (argc!=2 || (strcmp(argv[1],"--support") && strcmp(argv[1],"--synthetic"))) return 2;
 setvbuf(stdout,NULL,_IOLBF,0);
 int fd=open("/dev/" TT_PCI_PROBE_DEVICE,O_RDWR|O_CLOEXEC);
 if (fd<0) { perror("open P100A"); return 1; }
 tt_q38_delta_support support={};
 bool okay=DeltaCall(fd,TT_PCI_PROBE_Q38_DELTA_SUPPORT,&support,sizeof(support))
  && support.version==TT_Q38_DELTA_PROBE_VERSION && support.driver==55 && support.local_only==1 && support.keys==128
  && support.values==64 && support.state_words==8192 && support.accept_required==1;
 if (!okay || !strcmp(argv[1],"--support")) {
  if (okay) puts("LOCAL_DELTA_SUPPORT keys128 values64 state8192 ACCEPT_REQUIRED;NOT_MODEL_INFERENCE");
  close(fd); return okay?0:1;
 }
 tt_q38_bank_session session={}; session.version=1;
 okay=DeltaCall(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session)) && session.result.state==TT_RESIDENT_SESSION_READY
  && session.result.active && session.result.host_ready && !session.result.cold && !session.result.capabilities
  && session.result.bytes==PioneerResidentBanks::Total;
 unsigned cases=0;
 for (unsigned trial=0;trial<4 && okay;trial++) {
  PioneerQ38DeltaDebugFixture(deltaRequest,trial); deltaInputs=deltaRequest;
  okay=DeltaCall(fd,TT_PCI_PROBE_Q38_DELTA_RUN,&deltaRequest,sizeof(deltaRequest));
  if (!okay) break;
  const auto& r=deltaRequest.result;
  okay=r.version==TT_Q38_DELTA_PROBE_VERSION && r.state==TT_RUNTIME_PASSED && r.prepared==1 && r.started==1 && r.completed==1 && r.stopped==1
   && !r.recovery_required && r.nonce==trial+1 && r.phase==0x7200c && r.count==64 && r.marker==0x31564451
   && PioneerQ38DeltaProbeTerminal(r.pc[0]) && PioneerQ38DeltaProbeTerminal(r.pc[1])
   && !memcmp(&deltaInputs,&deltaRequest,offsetof(tt_q38_delta_probe,result));
  for (unsigned i=0;i<8;i++) okay &= r.before[i]==r.after[i];
  bool numericChecked=okay;
  unsigned errors=numericChecked?PioneerQ38DeltaDebugErrors(deltaRequest):UINT32_MAX; okay &= !errors;
  printf("LOCAL_DELTA trial%u state%u start/complete/stop=%u/%u/%u cold%u pc=%x/%x phase=%x count%u marker=%x numeric_checked%u exact_errors%u words8256\n",
   trial,r.state,r.started,r.completed,r.stopped,r.recovery_required,r.pc[0],r.pc[1],r.phase,r.count,r.marker,unsigned(numericChecked),errors);
  if (!okay) break; // NO ACCEPT/END/retry/reset after numeric or metadata failure
  tt_q38_delta_accept accept={TT_Q38_DELTA_PROBE_VERSION,r.nonce,{0,0}};
  okay=DeltaCall(fd,TT_PCI_PROBE_Q38_DELTA_ACCEPT,&accept,sizeof(accept)); if (okay) cases++;
 }
 if (!okay) {
  fprintf(stderr,"STOP:LOCAL_DELTA_FAILURE;CLOSE_ONLY_NO_ACCEPT_END_RETRY_OR_RESET\n"); close(fd); return 1;
 }
 session={}; session.version=1;
 okay=DeltaCall(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session)) && session.result.state==TT_RESIDENT_SESSION_CLOSED
  && !session.result.active && session.result.pci_restored && session.result.host_released && !session.result.cold;
 if (close(fd)) okay=false;
 printf("%s_LOCAL_VECTOR_DELTA cases%u KNOWN_CLEAN_END%u;NOT_RESIDENT_GGML_OR_MODEL_SPEED\n",okay?"PASS":"FAIL",cases,unsigned(okay));
 return okay?0:1;
}
