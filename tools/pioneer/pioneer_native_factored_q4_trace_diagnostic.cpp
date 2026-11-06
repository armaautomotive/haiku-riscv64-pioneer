/* SPDX-License-Identifier: MIT */
// Fixed CPU-reference TRACE DIAGNOSTIC. No Work/fallback or arbitrary operands.
#include <Drivers.h>
#include "../../src/add-ons/kernel/drivers/misc/PioneerNativeCompactQ4StreamProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "PioneerNativeCompactQ4StreamSource.h"
#define PIONEER_Q38_Q4_FACTORED_TRACE_REPORT 1
#include "PioneerQ38Q4FactoredTraceReport.h"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#if !defined(PIONEER_NATIVE_FACTORED_Q4_TRACE) || PIONEER_NATIVE_FACTORED_Q4_TRACE!=1 || !defined(PIONEER_NATIVE_FACTORED_Q4_TRACE_EXPECTED_GENERATION) || PIONEER_NATIVE_FACTORED_Q4_TRACE_EXPECTED_GENERATION<2
#error Exact trace112/fresh generation client required
#endif
namespace {
using V=PioneerNativeCompactQ4StreamVariant;using R=PioneerNativeCompactQ4StreamRequest;
constexpr uint64_t Generation=PIONEER_NATIVE_FACTORED_Q4_TRACE_EXPECTED_GENERATION;
bool Delivered(){return !fflush(stdout) && !ferror(stdout);}
bool Result(const R& r,bool closed){
 const auto& c=r.result.compute;const unsigned restored=unsigned(closed);
 if(r.version!=1 || r.sample || r.expected_tag!=V::Tag || r.expected_generation!=Generation || r.nonce!=1 || r.accept!=restored || r.reserved
  || c.state!=(closed?PIONEER_NATIVE_BFP_CLOSED:PIONEER_NATIVE_BFP_RESULT) || c.cold || c.started!=1 || c.completed!=1 || !c.polls || c.polls>20000
  || c.phase!=0x7000c || c.count!=0x04001400 || c.marker!=V::Marker || (c.controls[0]&1) || c.controls[1] || c.controls[2])return false;
 for(unsigned pc:c.pc)if(pc!=0x70038 && pc!=0x7003c)return false;
 for(unsigned value:r.result.timing)if(value)return false;
 return c.held==restored && c.l1_restored==restored && c.tlb_restored==restored && r.result.phase==(closed?10U:5U)
  && r.result.gddr_restored==restored && r.result.niu_restored==restored && r.result.clock_restored==restored
  && r.result.saved_l1==37136 && r.result.saved_gddr==25600 && r.result.requests==1040 && r.result.flits==3200;
}
}
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--fixed-q4-stage-trace")){fputs("Use --fixed-q4-stage-trace for isolated112;NOT Work/full-model inference.\n",stderr);return 2;}
 if(geteuid()!=0)return 1;
 PioneerNativeCompactQ4StreamSource source;std::string error;uint32_t expected[64]={};
 puts("FACTORED_TRACE_AUTHENTICATING_ORIGINAL_ALL_PAYLOADS_AND_STAGE_REFERENCE;NO_DEVICE_OPEN");if(!Delivered())return 1;
 if(!source.Prepare("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",error) || !source.TraceGolden(expected)){
  fprintf(stderr,"FACTORED_TRACE_SOURCE_REFUSED %s;NO_DEVICE_OPEN\n",error.c_str());return 1;
 }
 puts("FACTORED_TRACE_AUTHENTICATED c600de03 all9_originalPayloads_fixedSample0_stages;CPU_DIAGNOSTIC_NOT_WORK");if(!Delivered())return 1;
 const int fd=open("/dev/misc/tenstorrent_pci_probe",O_RDWR|O_CLOEXEC);if(fd<0)return 1;
 tt_pioneer_driver_reload metadata={};metadata.version=1;metadata.expected_tag=V::Tag;
 if(ioctl(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata)) || metadata.version!=1 || metadata.expected_tag || metadata.expected_generation
  || metadata.reserved[0] || metadata.reserved[1] || metadata.reserved[2] || metadata.driver!=55 || metadata.build_tag!=V::Tag || metadata.generation!=Generation
  || metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1 || metadata.guard_version!=1){
  fputs("FACTORED_TRACE_METADATA_REFUSED;NO_SUPPORT_OR_RUN\n",stderr);close(fd);return 1;
 }
 R support={};support.version=1;support.sample=PioneerNativeCompactQ4StreamNone;support.expected_tag=V::Tag;support.expected_generation=Generation;
 if(ioctl(fd,PIONEER_NATIVE_COMPACT_Q4_STREAM_SUPPORT,&support,sizeof(support)) || !PioneerNativeCompactQ4StreamRequestValid(support)
  || support.sample!=PioneerNativeCompactQ4StreamNone || support.nonce || support.accept || support.expected_generation!=Generation || support.result.compute.state!=PIONEER_NATIVE_BFP_EMPTY || support.result.compute.cold || !source.Ready()){
  fputs("FACTORED_TRACE_SUPPORT_REFUSED;NO_RUN\n",stderr);close(fd);return 1;
 }
 R r={};r.version=1;r.expected_tag=V::Tag;r.expected_generation=Generation;
 const int status=ioctl(fd,PIONEER_NATIVE_COMPACT_Q4_STREAM_RUN,&r,sizeof(r));
 const bool completed=!status && Result(r,false) && source.Ready();
 // Dump EVERY returned word BEFORE any numerical decision. Unproven
 // transport/result bytes are explicitly NOT promoted to hardware evidence.
 puts(completed?"FACTORED_TRACE_RETURNED_COMPLETED_DATA":"FACTORED_TRACE_RETURNED_UNPROVEN_DATA_NO_ACCEPTANCE");
 bool exact=false;const bool written=PioneerQ38FactoredTraceReport(stdout,r.result.compute.output,expected,exact);
 if(!completed || !written || !exact || !source.Ready()){
  fprintf(stderr,"FACTORED_TRACE_REJECTED status%d completed%u delivered%u exact%u state%u cold%u;CLOSE_ONLY_NO_END_RESET_RETRY\n",status,unsigned(completed),unsigned(written),unsigned(exact),r.result.compute.state,r.result.compute.cold);
  close(fd);return 1;
 }
 R end={};end.version=1;end.expected_tag=V::Tag;end.expected_generation=Generation;end.nonce=1;end.accept=1;
 if(ioctl(fd,PIONEER_NATIVE_COMPACT_Q4_STREAM_END,&end,sizeof(end)) || !Result(end,true)
  || memcmp(end.result.compute.output,r.result.compute.output,sizeof(r.result.compute.output))
  || memcmp(end.result.compute.controls,r.result.compute.controls,sizeof(r.result.compute.controls))
  || memcmp(end.result.timing,r.result.timing,sizeof(r.result.timing)) || end.result.compute.polls!=r.result.compute.polls || !source.Ready()){
  fputs("FACTORED_TRACE_END_UNPROVEN;CLOSE_ONLY_NO_RETRY\n",stderr);close(fd);return 1;
 }
 if(close(fd))return 1;
 printf("PASS_NATIVE_FACTORED_TRACE sample0 width5120 exactFinal32_trace32 generation%llu saved_l137136 saved_gddr25600 requests1040 flits3200;EXPLICIT_END_L1_GDDR_NIU_TLB_FIFO_CLOCK_PCI_RESTORED_NOT_MODEL_TPS\n",(unsigned long long)Generation);
 return Delivered()?0:1;
}
