/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 0x1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8TraceNativeProtocol.h"
#include <cassert>
#include <cstdio>
#include <memory>
int main(){
 namespace C=PioneerQ38TokenControl;namespace F=PioneerQ38FullOffload;namespace T=PioneerQ38Qgw8TraceNative;
 static_assert(F::BuildTag==UINT64_C(0x2026110700010131) && F::MaxPrompt==1 && F::MaxOutput==1 && F::MaxPositions==1,"exact trace identity");
 auto p=std::make_unique<C::Packet>();auto& q=p->request;
 q.magic=C::Magic;q.version=C::Version;q.operation=C::Start;q.prompts=q.limit=1;
 q.eos0=PioneerQ38FullOffloadEog::Eos0;q.eos1=PioneerQ38FullOffloadEog::Eos1;p->prompt[0]=248319;
 assert(F::RequestShape(*p) && T::ControlRequestShape(*p));
 q.stopEos=1;assert(!F::RequestShape(*p) && !T::ControlRequestShape(*p));q.stopEos=0;
 p->prompt[0]--;assert(F::RequestShape(*p) && !T::ControlRequestShape(*p));p->prompt[0]++;
 q.eos0++;assert(!F::RequestShape(*p));
 puts("PASS actual tag131 full+trace START intersection and exact support identity; NO device");
}
