/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#define B_DEVICE_OP_CODES_END 10000
#define TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR
#define PIONEER_Q38_ROOT_STATS_BENCH
#include "../PioneerQ38RootStatsProfile.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h"
struct Mode {
#include "root_stats_work_support.inc"
};
struct IO {
 unsigned metadataCalls=0,supportCalls=0,fault=0;uint64_t tag=0;
 bool Call(int fd,uint32_t op,void* data,size_t bytes) {
  assert(fd==17);
  if(op==TT_PCI_PROBE_RELOAD_STATUS) {
   metadataCalls++;assert(bytes==sizeof(tt_pioneer_driver_reload));
   auto& r=*static_cast<tt_pioneer_driver_reload*>(data);assert(r.version==1);
   memset(&r,0,sizeof(r));r.version=1;r.driver=55;r.guard_version=1;
   r.build_tag=tag;r.generation=23;r.open_count=1;
   if(fault==1)return false;
   if(fault==2)r.build_tag--;
   if(fault==3)r.active_owner=1;
   if(fault==4)r.cold=1;
   if(fault==5)r.open_count=2;
   return true;
  }
  assert(op==TT_PCI_PROBE_Q38_WORK_SUPPORT && metadataCalls==1);
  supportCalls++;assert(bytes==sizeof(tt_q38_dispatch_support));
  auto& s=*static_cast<tt_q38_dispatch_support*>(data);memset(&s,0,sizeof(s));
  s.driver=TT_Q38_DISPATCH_DRIVER;s.version=TT_Q38_WORK_SUPPORT_VERSION;
  s.peers=119;s.rows=256;s.max_inputs=17408;s.banks=7;s.offset_alignment=64;s.persistent=1;
  if(fault==6)return false;
  if(fault==7)s.version=3; // Native SM7 MUST NOT match this original-TC profile
  if(fault==8)s.model_inference=1;
  return true;
 }
};
int main() {
 for(uint64_t tag:{UINT64_C(0x2026102100010012),UINT64_C(0x2026102100010016)})for(unsigned fault=0;fault<=8;fault++) {
  IO io;io.fault=fault;io.tag=tag;Mode mode;
  assert(mode.Support(io,17)==!fault && io.metadataCalls==1);
  assert(io.supportCalls==(fault>=1 && fault<=5?0U:1U));
 }
 puts("PASS ACTUAL extracted root-stats Work support:exact idle tag12/16 BEFORE original-TC ABI2 query;failed metadata/tag/owner/cold/open-count/transport/SM7 ABI/model capability refuse;NO BEGIN/accelerator IO");
}
