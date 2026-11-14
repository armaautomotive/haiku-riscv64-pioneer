/* SPDX-License-Identifier: MIT */
// Actual extracted routing, with inert host-only handlers; no device access.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#define B_DEVICE_OP_CODES_END 0x1000
#define TT_PIONEER_Q38_WORK_MODE 1
#define TT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkBatchProtocol.h"
enum { B_NOT_ALLOWED=-6,NotHandled=-999 };
static unsigned calls=0;
static int Q38DispatchControl(void*,unsigned,void*,unsigned){calls++;return 42;}
#ifdef PIONEER_Q38_WORK_BATCH_NATIVE_152
static int Q38WorkBatchSupport(void*,void*,unsigned){calls++;return 43;}
static int Q38WorkBatchControlInspection(void*,void*,unsigned){calls++;return 44;}
#endif
static int Route(unsigned operation){void* instance=nullptr;void* data=nullptr;unsigned length=0;
#include PIONEER_WORK_BATCH_ROUTING_INCLUDE
 return NotHandled;
}
int main(){
 for(unsigned op:{unsigned(TT_PCI_PROBE_Q38_WORK_SUPPORT),unsigned(TT_PCI_PROBE_Q38_WORK_RUN)}){
  calls=0;assert(Route(op)==42 && calls==1);
 }
 calls=0;
#ifdef PIONEER_Q38_WORK_BATCH_NATIVE_152
 assert(Route(TT_PCI_PROBE_Q38_WORK_BATCH_SUPPORT)==43 && calls==1);calls=0;
 assert(Route(TT_PCI_PROBE_Q38_WORK_BATCH_RUN)==44 && calls==1);
#else
 assert(Route(TT_PCI_PROBE_Q38_WORK_BATCH_SUPPORT)==NotHandled && !calls);
 assert(Route(TT_PCI_PROBE_Q38_WORK_BATCH_RUN)==NotHandled && !calls);
#endif
 puts("PASS actual Work routing with and without isolated batch selector; NO DEVICE");
}
