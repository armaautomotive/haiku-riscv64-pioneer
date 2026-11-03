/* SPDX-License-Identifier: MIT */
// Explicit fixed read diagnostic only;NO install/reset/model/CPU-reference.
#include <Drivers.h>
#include <OS.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDramCohortProtocol.h"
static bool Number(const char* p,unsigned& v){
 if(!p || !*p)return false;
 v=0;
 for(;*p;p++){if(*p<'0' || *p>'9' || v>256)return false;v=v*10+unsigned(*p-'0');}
 return v<=256;
}
int main(int argc,char** argv){
 const bool support=argc==2 && !strcmp(argv[1],"--support");
 const bool preflight=argc==2 && !strcmp(argv[1],"--preflight");
 const bool run=argc==4 && !strcmp(argv[1],"--run");
 tt_pioneer_dram_cohort r={};r.version=1;r.sweeps=1;r.queue_depth=1;
 if((!support && !preflight && !run) || (run && (!Number(argv[2],r.sweeps) || !Number(argv[3],r.queue_depth))))return 2;
 r.nonce=uint32_t(system_time())^(uint32_t(getpid())<<13);if(!r.nonce)r.nonce=1;
 r.expected_tag=UINT64_C(0x2026110300010077);r.expected_generation=1;
 if(!PioneerDramCohortRequest(r))return 2;
 const int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);
 if(fd<0){perror("STOP cohort diagnostic open");return 1;}
 tt_pioneer_driver_reload state={};state.version=1;
 if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&state,sizeof(state)) || !PioneerDriverReloadRequest(state)
  || state.expected_tag || state.expected_generation
  || state.build_tag!=r.expected_tag || !state.generation || state.driver!=55 || state.guard_version!=1
  || state.cold || state.quiesced || state.active_owner || state.open_count!=1){
  puts("STOP exact idle cohort77 metadata required;NO execution or retry");close(fd);return 1;
 }
 r.expected_generation=state.generation;
 const uint32 op=support?TT_PCI_PROBE_DRAM_COHORT_SUPPORT:preflight?TT_PCI_PROBE_DRAM_COHORT_PREFLIGHT:TT_PCI_PROBE_DRAM_COHORT_RUN;
 const bigtime_t began=system_time();errno=0;
 const int result=ioctl(fd,op,&r,sizeof(r));const int error=errno;
 const bigtime_t ended=system_time();const int closed=close(fd);
 const auto& p=r.reply;const auto& t=p.timing;
 printf("COHORT_DIAGNOSTIC tag%016llx generation%llu operation%s sweeps%u depth%u ioctl%d errno%d close%d host_us%lld "
  "version%u phase%u retained%u complete%u preflight%u busy_mhz%u idle_mhz%u views%u pci%u reservation%u\n",
  (unsigned long long)r.expected_tag,(unsigned long long)r.expected_generation,support?"support":preflight?"preflight":"run",
  r.sweeps,r.queue_depth,result,error,closed,(long long)(ended-began),p.version,p.phase,p.retained,p.complete,p.preflight,
  p.busy_mhz,p.idle_mhz,p.views_closed,p.pci_closed,p.reservation_closed);
 for(unsigned i=0;i<268;i++)printf("COHORT_WORD %u %08x\n",i,p.sweep[i]);
 printf("COHORT_TIMING version%u state%u start_us%llu..%llu end_us%llu..%llu elapsed_us%llu..%llu cycles%llu bytes%llu\n",
  t.version,t.state,(unsigned long long)t.start_lower_us,(unsigned long long)t.start_upper_us,
  (unsigned long long)t.end_lower_us,(unsigned long long)t.end_upper_us,
  (unsigned long long)t.elapsed_min_us,(unsigned long long)t.elapsed_max_us,(unsigned long long)t.cycles,(unsigned long long)t.bytes);
 const bool okay=!result && !closed && PioneerDramCohortReply(r,op);
 if(okay && run && t.state==1){
  printf("HOST_CLOCK_BRACKET_GBPS %.9f..%.9f COUNTER_GHZ %.9f..%.9f\n",
   double(t.bytes)/double(t.elapsed_max_us)/1000.0,double(t.bytes)/double(t.elapsed_min_us)/1000.0,
   double(t.cycles)/double(t.elapsed_max_us)/1000.0,double(t.cycles)/double(t.elapsed_min_us)/1000.0);
 }else puts("NO_ACCEPTED_BANDWIDTH_ESTIMATE");
 puts("ONE_COORDINATOR_INTERVAL;NO_SUM_OF_READER_CLOCKS;NOT_PEAK_CAPACITY_AUTHENTICITY_OR_MODEL_TPS_PROOF");
 return okay?0:1;
}
