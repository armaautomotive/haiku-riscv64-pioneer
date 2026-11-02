/* SPDX-License-Identifier: MIT */
// Explicit diagnostic only. No model/CPU-reference/driver install/reset route.
#include <Drivers.h>
#include <OS.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDramSweepProtocol.h"
static bool Number(const char* text,unsigned& value){
 if(!text || !*text)return false;
 value=0;
 for(const auto* p=text;*p;p++){
  if(*p<'0' || *p>'9' || value>256)return false;
  value=value*10+unsigned(*p-'0');
 }
 return value<=256;
}
int main(int argc,char** argv){
 const bool support=argc==2 && !strcmp(argv[1],"--support");
 const bool preflight=argc==2 && !strcmp(argv[1],"--preflight");
 const bool run=argc==6 && !strcmp(argv[1],"--run");
 tt_pioneer_dram_sweep r={};r.version=3;r.chunk_shift=14;r.sweeps=1;r.queue_depth=1;
 if((!support && !preflight && !run) || (run && (!Number(argv[2],r.bank)
  || !Number(argv[3],r.chunk_shift) || !Number(argv[4],r.sweeps) || !Number(argv[5],r.queue_depth))))return 2;
 r.expected_tag=UINT64_C(0x2026110200050076);
 r.nonce=uint32_t(system_time())^(uint32_t(getpid())<<13);if(!r.nonce)r.nonce=1;
 // Shape-check before opening even a metadata FD; generation is queried below.
 r.expected_generation=1;if(!PioneerDramSweepRequest(r))return 2;
 const int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);
 if(fd<0){perror("STOP diagnostic open");return 1;}
 tt_pioneer_driver_reload state={};state.version=1;
 if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&state,sizeof(state)) || state.version!=1
  || state.build_tag!=r.expected_tag || !state.generation || state.driver!=55
  || state.guard_version!=1 || state.cold || state.quiesced || state.active_owner || state.open_count!=1){
  puts("STOP exact idle diagnostic76 metadata required;NO execution or retry");close(fd);return 1;
 }
 r.expected_generation=state.generation;
 const uint32 operation=support?TT_PCI_PROBE_DRAM_SWEEP_SUPPORT:
  preflight?TT_PCI_PROBE_DRAM_SWEEP_PREFLIGHT:TT_PCI_PROBE_DRAM_SWEEP_RUN;
 const bigtime_t began=system_time();
 errno=0;
 const int result=ioctl(fd,operation,&r,sizeof(r));const int error=errno;
 const bigtime_t ended=system_time();const int closed=close(fd);
 const auto& p=r.reply;
 printf("DRAM_DIAGNOSTIC tag%016llx generation%llu operation%s depth%u ioctl%d errno%d close%d host_us%lld "
  "version%u phase%u retained%u complete%u preflight%u busy_mhz%u idle_mhz%u views%u pci%u reservation%u\n",
  (unsigned long long)r.expected_tag,(unsigned long long)r.expected_generation,
  support?"support":preflight?"preflight":"run",r.queue_depth,result,error,closed,(long long)(ended-began),
  p.version,p.phase,p.retained,p.complete,p.preflight,p.busy_mhz,p.idle_mhz,
  p.views_closed,p.pci_closed,p.reservation_closed);
 for(unsigned i=0;i<28;i++)printf("SWEEP_WORD %u %08x\n",i,p.sweep[i]);
 puts("RAW_DEVICE_CYCLES_ONLY;FIRMWARE_MHZ_NOT_INDEPENDENT_COUNTER_FREQUENCY_PROOF;NOT_FULL_CARD_BANDWIDTH_OR_MODEL_TPS");
 return !result && !closed && PioneerDramSweepReply(r,operation)?0:1;
}
