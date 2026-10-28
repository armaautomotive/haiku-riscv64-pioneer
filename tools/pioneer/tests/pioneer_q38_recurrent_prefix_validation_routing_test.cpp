/* SPDX-License-Identifier: MIT */
// ONLY routing/admission code is instantiated. Handler/OS endpoints are
// explicit stubs,not native producers,Source/clock scopes or device access.
#define B_DEVICE_OP_CODES_END 1000
#include <cstddef>
#define PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT 1
#if PREFIX_ROUTING_PHYSICAL
#define PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE 1
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixValidationProtocol.h"
#include <array>
#include <cassert>
#include <cstdio>
using uint32=uint32_t;using status_t=int;
static constexpr int B_NOT_ALLOWED=-5;
struct PioneerProbeCookie {uint32 magic=0x50313030;};
static unsigned calls[4]={};static int uid=0;
static status_t DriverReloadControl(PioneerProbeCookie*,uint32,void*,size_t){calls[0]++;return 101;}
static status_t Q38BankControl(PioneerProbeCookie*,uint32,void*,size_t){calls[1]++;return 102;}
static status_t Q38SpanControl(PioneerProbeCookie*,uint32,void*,size_t){calls[2]++;return 103;}
[[maybe_unused]] static status_t Q38RecurrentPrefixValidationControl(PioneerProbeCookie*,uint32,void*,size_t){calls[3]++;return 104;}
#define geteuid() uid
#include "prefix_actual_routing.inc"
#undef geteuid
static unsigned Category(unsigned op){
 if(op==TT_PCI_PROBE_RELOAD_STATUS || op==TT_PCI_PROBE_RELOAD_QUIESCE)return 1;
 if(op==TT_PCI_PROBE_Q38_BANK_SUPPORT || op==TT_PCI_PROBE_Q38_BANK_BEGIN || op==TT_PCI_PROBE_Q38_BANK_END)return 2;
 if(op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT || op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || op==TT_PCI_PROBE_Q38_SPAN_END)return 3;
#if PREFIX_ROUTING_PHYSICAL
 if(op==TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATION_SUPPORT || op==TT_PCI_PROBE_Q38_RECURRENT_PREFIX_VALIDATE)return 4;
#endif
 return 0;
}
int main(){
 PioneerProbeCookie cookie;unsigned allowed=0,denied=0;
 for(unsigned op=0;op<65536;op++){
  std::array<unsigned,4> before;for(unsigned i=0;i<4;i++)before[i]=calls[i];
  auto category=Category(op);assert(ProbeControl(&cookie,op,nullptr,0)==(category?100+int(category):B_NOT_ALLOWED));
  for(unsigned i=0;i<4;i++)assert(calls[i]==before[i]+unsigned(category==i+1));
  if(category)allowed++;else denied++;
 }
 assert(allowed==(PREFIX_ROUTING_PHYSICAL?10U:8U));
 const auto previous=std::array<unsigned,4>{calls[0],calls[1],calls[2],calls[3]};
 uid=1;for(unsigned op=0;op<65536;op++)assert(ProbeControl(&cookie,op,nullptr,0)==B_NOT_ALLOWED);uid=0;
 for(unsigned op=0;op<65536;op++){
  assert(ProbeControl(nullptr,op,nullptr,0)==B_NOT_ALLOWED);cookie.magic=0;
  assert(ProbeControl(&cookie,op,nullptr,0)==B_NOT_ALLOWED);cookie.magic=0x50313030;
 }
 for(unsigned i=0;i<4;i++)assert(calls[i]==previous[i]);
 printf("PASS VERBATIM prefix %s routing/%u allowed/%u denied opcodes/ALL65536 wrongUID+nullFD+badMagic refusals before handlers;NO old layer0,dense,Work or arbitrary-address fallback;HANDLER STUBS ONLY,NOT native/runtime/physical qualification\n",PREFIX_ROUTING_PHYSICAL?"physical73":"NODRV",allowed,denied);
}
