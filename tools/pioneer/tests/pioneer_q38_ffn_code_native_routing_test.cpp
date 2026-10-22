/* SPDX-License-Identifier: MIT */
// ACTUAL extracted operation routing;NO native device/cookie/capability.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#define B_DEVICE_OP_CODES_END 0x1000
#define TT_PIONEER_Q38_WORK_MODE
#define TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h"
enum {B_NOT_ALLOWED=-6,NotHandled=-999};
static unsigned calls=0,lastOperation=0;
static int Q38DispatchControl(void*,unsigned op,void*,unsigned){calls++;lastOperation=op;return 42;}
static int ActualRoute(unsigned operation){void* instance=nullptr;void* data=nullptr;unsigned length=0;
#include PIONEER_FFN_NATIVE_ROUTING_INCLUDE
 return NotHandled;
}
int main(){unsigned tested=0;
 for(unsigned op:{unsigned(TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT),unsigned(TT_PCI_PROBE_Q38_LEGACY_WORK_RUN),
                  unsigned(TT_PCI_PROBE_Q38_DISPATCH_SUPPORT),unsigned(TT_PCI_PROBE_Q38_DISPATCH_RUN)}){
  calls=0;int result=ActualRoute(op);
#ifdef PIONEER_Q38_FFN_CODE_DIAGNOSTIC
  assert(result==B_NOT_ALLOWED && !calls);
#else
  assert(result==42 && calls==1 && lastOperation==op);
#endif
  tested++;
 }
 for(unsigned op:{0x7270U,0x7271U}){calls=0;int result=ActualRoute(op);
#ifdef PIONEER_Q38_FFN_CODE_DIAGNOSTIC
  assert(result==42 && calls==1 && lastOperation==op && TT_Q38_WORK_SUPPORT_VERSION==5);
#else
  assert(result==NotHandled && !calls && TT_Q38_WORK_SUPPORT_VERSION==2);
#endif
  tested++;
 }
 for(unsigned op:{0x7250U,0x7251U,0x7260U,0x7261U,0U,UINT32_MAX}){calls=0;assert(ActualRoute(op)==NotHandled && !calls);tested++;}
 printf("PASS ACTUAL kernel routing:operations%u/ordinary Work+checked unchanged/private code qualifier excludes BOTH ordinary interfaces/NO fallback or IO\n",tested);
}
