/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 0x1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8TraceNativeProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8HiddenProtocol.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <initializer_list>
int main(){
 namespace C=PioneerQ38TokenControl;namespace T=PioneerQ38Qgw8TraceNative;
 static_assert(TT_PCI_PROBE_Q38_QGW8_TRACE_READ!=TT_PCI_PROBE_Q38_QGW8_HIDDEN_READ,"distinct trace opcode");
 auto p=std::make_unique<C::Packet>();auto start=[&](){*p={};p->request.magic=C::Magic;p->request.version=C::Version;
  p->request.operation=C::Start;p->request.prompts=p->request.limit=1;p->prompt[0]=248319;};
 start();assert(T::ControlRequestShape(*p));
 p->prompt[0]--;assert(!T::ControlRequestShape(*p));start();p->request.limit=2;assert(!T::ControlRequestShape(*p));
 start();p->request.prompts=0;assert(!T::ControlRequestShape(*p));start();p->request.stopEos=1;assert(!T::ControlRequestShape(*p));
 *p={};p->request.magic=C::Magic;p->request.version=C::Version;p->request.operation=C::Read;p->request.records=1;
 assert(T::ControlRequestShape(*p));p->request.records=0;assert(!T::ControlRequestShape(*p));
 p->request.records=2;assert(!T::ControlRequestShape(*p));p->request.records=0;p->request.operation=C::Cancel;assert(!T::ControlRequestShape(*p));
 for(unsigned op:{unsigned(C::Observe),unsigned(C::Hold),unsigned(C::Finish)}){p->request.operation=op;assert(T::ControlRequestShape(*p));}
 p->request.reserved[0]=1;assert(!T::ControlRequestShape(*p));
 puts("PASS tag131 fixed-input/one-position/no-cancel protocol DATA; NO driver or device");
}
