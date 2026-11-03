/* SPDX-License-Identifier: MIT */
// Execute the ACTUAL CLI against syscall endpoints. Synthetic receipts only;
// no Haiku kernel,model,card,driver install or measured bandwidth.
#include <Drivers.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <string>
#include <vector>
using uint32=uint32_t;using bigtime_t=int64_t;
#ifdef COHORT_DIRECT_ENDPOINT_TEST
#define PIONEER_DRAM_COHORT_DIRECT_TLB 1
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDramCohortProtocol.h"
static unsigned opens=0,requests=0,closes=0,fault=0,operation=0;
static bigtime_t timeValue=1000000;
static bigtime_t TestTime(){return timeValue++;}
static int TestOpen(const char* name,int mode){
 assert(!strcmp(name,"/dev/misc/tenstorrent_pci_probe") && mode==(O_RDWR|O_CLOEXEC));opens++;
 if(fault==14){errno=ENODEV;return -1;}return 9;
}
static int TestClose(int fd){assert(fd==9);closes++;return fault==11?-1:0;}
static void Fill(tt_pioneer_dram_cohort& r){
 auto& p=r.reply;p={};p.version=6;p.sweep[0]=6;p.timing.version=1;
 if(operation==TT_PCI_PROBE_DRAM_COHORT_SUPPORT)return;
 p.phase=3;p.views_closed=p.pci_closed=p.reservation_closed=1;
 if(operation==TT_PCI_PROBE_DRAM_COHORT_PREFLIGHT){p.preflight=1;return;}
 assert(operation==TT_PCI_PROBE_DRAM_COHORT_RUN);
 p.complete=1;p.busy_mhz=1350;p.idle_mhz=800;
 auto* s=p.sweep;s[1]=s[7]=1;s[2]=10;s[4]=s[5]=s[6]=255;
 s[8]=s[9]=493457;s[10]=7*r.queue_depth*4096;
 const unsigned routes[7]={18|(12<<6),18|(15<<6),18|(18<<6),17|(12<<6),17|(15<<6),17|(18<<6),17|(21<<6)};
 for(unsigned role=0;role<8;role++){
  auto* v=s+11+role*32;v[0]=0x36435244;v[1]=6;v[2]=role;v[3]=r.nonce;v[4]=r.sweeps;v[5]=r.queue_depth;
  const uint64_t bytes=UINT64_C(8388608)*r.sweeps*(role?1:7);
  v[6]=uint32(bytes);v[7]=uint32(bytes>>32);v[8]=50;v[10]=1000050;v[12]=1000000;
  v[14]=role?routes[role-1]:28;v[15]=512*r.sweeps*(role?1:7);
  if(!role){v[16]=7;v[17]=42;for(unsigned k=1;k<=7;k++)v[16+k*2]=200*k;}
 }
 auto& t=p.timing;t.state=1;t.start_lower_us=10;t.start_upper_us=20;t.end_lower_us=1000;t.end_upper_us=1010;
 t.elapsed_min_us=980;t.elapsed_max_us=1000;t.cycles=1000000;t.bytes=UINT64_C(7)*8388608*r.sweeps;
 if(fault==12)s[4]=254;
 if(fault==13){t.state=2;t.elapsed_min_us=t.elapsed_max_us=0;}
}
static int TestIoctl(int fd,unsigned long op,void* data,size_t bytes){
 assert(fd==9 && !closes);requests++;
 if(op==TT_PCI_PROBE_RELOAD_STATUS){
  assert(requests==1 && bytes==sizeof(tt_pioneer_driver_reload));auto& s=*static_cast<tt_pioneer_driver_reload*>(data);
  assert(s.version==1);s={};s.version=1;s.driver=55;s.build_tag=PIONEER_DRAM_COHORT_PROTOCOL_TAG;
  s.generation=7;s.guard_version=s.open_count=1;
  if(fault==1)s.build_tag^=1;
  if(fault==2)s.cold=1;
  if(fault==3)s.quiesced=1;
  if(fault==4)s.active_owner=1;
  if(fault==5)s.open_count=2;
  if(fault==6)s.guard_version=2;
  if(fault==7)s.generation=0;
  if(fault==8)s.reserved[1]=1;
  if(fault==9)s.expected_tag=1;
  return 0;
 }
 assert(requests==2 && bytes==sizeof(tt_pioneer_dram_cohort));operation=unsigned(op);
 auto& r=*static_cast<tt_pioneer_dram_cohort*>(data);assert(PioneerDramCohortRequest(r) && r.expected_generation==7);
 if(fault==10){errno=EIO;return -1;}
 Fill(r);return 0;
}
#define main CohortDiagnosticMain
#define open TestOpen
#define close TestClose
#define ioctl TestIoctl
#define system_time TestTime
#include "../pioneer_dram_cohort.cpp"
#undef system_time
#undef ioctl
#undef close
#undef open
#undef main
static int Call(std::vector<std::string> args,std::string& output){
 opens=requests=closes=operation=0;timeValue=1000000;
 std::vector<char*> argv;for(auto& s:args)argv.push_back(s.data());argv.push_back(nullptr);
 FILE* capture=tmpfile();assert(capture);fflush(stdout);const int saved=dup(1);assert(saved>=0);
 assert(dup2(fileno(capture),1)==1);
 const int result=CohortDiagnosticMain(int(args.size()),argv.data());fflush(stdout);rewind(capture);
 output.clear();char buffer[512];size_t n;
 while((n=fread(buffer,1,sizeof(buffer),capture)))output.append(buffer,n);
 assert(dup2(saved,1)==1);::close(saved);fclose(capture);return result;
}
int main(){
 unsigned good=0,bad=0;std::string output;
 for(const auto& args:std::vector<std::vector<std::string>>{{},{"test"},{"test","--work"},{"test","--reset"},
  {"test","--run"},{"test","--run","1","3"},{"test","--run","0","1"},{"test","--run","257","1"},
  {"test","--run","-1","1"},{"test","--run","99999999999","1"},{"test","--run","1","17"},
  {"test","--support","extra"}}){
  fault=0;assert(Call(args,output)==2 && !opens && !requests && !closes);bad++;
 }
 for(const auto& args:std::vector<std::vector<std::string>>{{"test","--support"},{"test","--preflight"},
  {"test","--run","1","1"},{"test","--run","256","16"}}){
  fault=0;assert(Call(args,output)==0 && opens==1 && requests==2 && closes==1);
  const bool estimate=output.find("HOST_CLOCK_BRACKET_GBPS")!=std::string::npos;
  assert(estimate==(operation==TT_PCI_PROBE_DRAM_COHORT_RUN));good++;
 }
 for(fault=1;fault<=14;fault++){
  const int result=Call({"test","--run","1","16"},output);
  if(fault==13){assert(!result && requests==2 && closes==1);good++;}
  else{assert(result==1 && opens==1 && requests==(fault<10?1:fault==14?0:2)
   && closes==(fault==14?0:1));bad++;}
  assert(output.find("HOST_CLOCK_BRACKET_GBPS")==std::string::npos);
 }
 printf("PASS_COHORT_ACTUAL_CLIENT valid%u refused%u invalidBeforeOpen1 exactMetadata1 noRetry1 closeOnce1 unavailableNeverPrintsRate1;SYSCALL_FIXTURES_SYNTHETIC_RECEIPTS_NOT_NATIVE_PHYSICAL_OR_MODEL_TPS\n",good,bad);
}
