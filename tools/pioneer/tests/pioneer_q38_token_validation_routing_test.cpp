/* SPDX-License-Identifier: MIT */
// Handler stubs exercise ONLY the actual routing and UID/FD admission.
// This fixture cannot select the native physical role or execute a model.
#define B_DEVICE_OP_CODES_END 1000
#include <cstddef>
#define PIONEER_Q38_TOKEN_NATIVE_CONTEXT 1
#if TOKEN_ROUTING_PHYSICAL
#define PIONEER_Q38_TOKEN_VALIDATION_NATIVE 1
#endif
#include "../../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenValidationProtocol.h"
#include <array>
#include <cassert>
#include <cstdio>
using uint32=uint32_t;using status_t=int;
static constexpr int B_NOT_ALLOWED=-5;
struct PioneerProbeCookie {uint32 magic=0x50313030;};
static unsigned calls[4]={};static int uid=0;
static void* expectedData=nullptr;static size_t expectedLength=0;
static status_t Handler(unsigned category,void* data,size_t length){
 assert(data==expectedData && length==expectedLength);calls[category]++;return 101+category;
}
static status_t DriverReloadControl(PioneerProbeCookie*,uint32,void* data,size_t length){return Handler(0,data,length);}
static status_t Q38BankControl(PioneerProbeCookie*,uint32,void* data,size_t length){return Handler(1,data,length);}
static status_t Q38SpanControl(PioneerProbeCookie*,uint32,void* data,size_t length){return Handler(2,data,length);}
[[maybe_unused]] static status_t Q38TokenNativeControl(PioneerProbeCookie*,void* data,size_t length){return Handler(3,data,length);}
#define geteuid() uid
#include "token_actual_routing.inc"
#undef geteuid
static unsigned Category(unsigned op){
 if(op==TT_PCI_PROBE_RELOAD_STATUS || op==TT_PCI_PROBE_RELOAD_QUIESCE)return 1;
 if(op==TT_PCI_PROBE_Q38_BANK_SUPPORT || op==TT_PCI_PROBE_Q38_BANK_BEGIN || op==TT_PCI_PROBE_Q38_BANK_END)return 2;
 if(op==TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT || op==TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD || op==TT_PCI_PROBE_Q38_SPAN_END)return 3;
#if TOKEN_ROUTING_PHYSICAL
 if(op==TT_PCI_PROBE_Q38_TOKEN_VALIDATE)return 4;
#endif
 return 0;
}
int main(){
 PioneerProbeCookie cookie;unsigned allowed=0,denied=0;unsigned marker=17;
 expectedData=&marker;expectedLength=sizeof(marker);
 for(unsigned op=0;op<65536;op++){
  const auto before=std::array<unsigned,4>{calls[0],calls[1],calls[2],calls[3]};
  const auto category=Category(op);
  assert(ProbeControl(&cookie,op,expectedData,expectedLength)==(category?100+int(category):B_NOT_ALLOWED));
  for(unsigned i=0;i<4;i++)assert(calls[i]==before[i]+unsigned(category==i+1));
  if(category)allowed++;else denied++;
 }
 assert(allowed==(TOKEN_ROUTING_PHYSICAL?9U:8U));
 const auto before=std::array<unsigned,4>{calls[0],calls[1],calls[2],calls[3]};
 for(unsigned op=0;op<65536;op++){
  uid=1;assert(ProbeControl(&cookie,op,expectedData,expectedLength)==B_NOT_ALLOWED);uid=0;
  assert(ProbeControl(nullptr,op,expectedData,expectedLength)==B_NOT_ALLOWED);
  cookie.magic=0;assert(ProbeControl(&cookie,op,expectedData,expectedLength)==B_NOT_ALLOWED);cookie.magic=0x50313030;
 }
 for(unsigned i=0;i<4;i++)assert(calls[i]==before[i]);
 std::printf("PASS actual token %s routing/%u allowed/%u denied selectors/all65536 wrongUID+nullFD+badMagic refusals/pointer+length passed unchanged/NO Work fallback;HANDLER STUBS ONLY,not native lifecycle,model,physical or TPS proof\n",TOKEN_ROUTING_PHYSICAL?"tag74":"NODRV",allowed,denied);
}
