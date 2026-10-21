/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_MODEL_ROOT_PROFILE
#define B_DEVICE_OP_CODES_END 10000
#define TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR
#include <cstddef>
#include "../PioneerQ38ModelRootProfile.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
struct Mode {
#include "model_root_support.inc"
};
struct IO {
 unsigned calls=0,fault=0;uint64_t tag=0;
 bool Call(int fd,uint32_t op,void* data,size_t bytes) {
  assert(fd==17);calls++;
  if(op==TT_PCI_PROBE_RELOAD_STATUS) {
   assert(calls==1 && bytes==sizeof(tt_pioneer_driver_reload));auto& r=*static_cast<tt_pioneer_driver_reload*>(data);
   assert(r.version==1);r={};r.version=1;r.driver=55;r.guard_version=1;r.generation=40;r.open_count=1;r.build_tag=tag;
   if(fault==1)return false;if(fault==2)r.cold=1;if(fault==3)r.active_owner=1;if(fault==4)r.open_count=2;if(fault==5)r.build_tag=UINT64_C(0x2026102100010018);
   return true;
  }
  assert(calls==2 && op==TT_PCI_PROBE_Q38_WORK_SUPPORT && bytes==sizeof(tt_q38_dispatch_support));
  auto& s=*static_cast<tt_q38_dispatch_support*>(data);s={};s.driver=TT_Q38_DISPATCH_DRIVER;s.version=TT_Q38_WORK_SUPPORT_VERSION;
  s.peers=119;s.rows=256;s.max_inputs=17408;s.banks=7;s.offset_alignment=64;s.persistent=1;
  if(fault==6)return false;if(fault==7)s.version=3;if(fault==8)s.model_inference=1;return true;
 }
};
static unsigned Arguments(bool repeatedRun,unsigned limit,bool graph) {
#include "model_root_arguments.inc"
 return 0;
}
int main(){
 unsigned cases=0;
 for(uint64_t tag:{UINT64_C(0x2026102100010012),UINT64_C(0x2026102100010017),UINT64_C(0x2026102100010019),UINT64_C(0x2026102100010020)})for(unsigned fault=0;fault<=8;fault++) {
  IO io;io.tag=tag;io.fault=fault;Mode mode;assert(mode.Support(io,17)==!fault && io.calls==(fault>=1 && fault<=5?1U:2U));cases++;
 }
 for(bool repeat:{false,true})for(unsigned limit:{0U,1U,4U,31U,32U,33U})for(bool graph:{false,true}) {
  assert(Arguments(repeat,limit,graph)==(repeat && limit==32 && !graph?0U:2U));cases++;
 }
 printf("PASS %u ACTUAL extracted model root metadata-before-original-TC-ABI2 and fixed THREE32 argument guards;bad owner/tag/cold/SM7/model capability refuses BEFORE BEGIN/card reads\n",cases);
}
