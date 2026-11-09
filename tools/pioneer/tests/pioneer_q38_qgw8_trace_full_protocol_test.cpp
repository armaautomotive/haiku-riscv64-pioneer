/* SPDX-License-Identifier: MIT */
#define B_DEVICE_OP_CODES_END 0x1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38HybridFullProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8TraceNativeProtocol.h"
#include <cassert>
#include <cstdio>
#include <memory>
int main(){
 namespace C=PioneerQ38TokenControl;namespace F=PioneerQ38FullOffload;namespace T=PioneerQ38Qgw8TraceNative;
#ifdef PIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134
 static_assert(F::BuildTag==UINT64_C(0x2026110700010134),"exact dense-value identity");
#elif defined(PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133)
 static_assert(F::BuildTag==UINT64_C(0x2026110700010133),"exact dense-input identity");
#elif defined(PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132)
 static_assert(F::BuildTag==UINT64_C(0x2026110700010132),"exact dense-stage identity");
#else
 static_assert(F::BuildTag==UINT64_C(0x2026110700010131),"exact trace identity");
#endif
 static_assert(F::MaxPrompt==1 && F::MaxOutput==1 && F::MaxPositions==1,"exact trace bounds");
 auto p=std::make_unique<C::Packet>();auto& q=p->request;
 q.magic=C::Magic;q.version=C::Version;q.operation=C::Start;q.prompts=q.limit=1;
 q.eos0=PioneerQ38FullOffloadEog::Eos0;q.eos1=PioneerQ38FullOffloadEog::Eos1;p->prompt[0]=248319;
 assert(F::RequestShape(*p) && T::ControlRequestShape(*p));
 q.stopEos=1;assert(!F::RequestShape(*p) && !T::ControlRequestShape(*p));q.stopEos=0;
 p->prompt[0]--;assert(F::RequestShape(*p) && !T::ControlRequestShape(*p));p->prompt[0]++;
 q.eos0++;assert(!F::RequestShape(*p));
#ifdef PIONEER_Q38_QGW8_DENSE_VALUE_NATIVE_134
 puts("PASS actual tag134 full+trace START intersection and exact support identity; NO device");
#elif defined(PIONEER_Q38_QGW8_DENSE_INPUT_NATIVE_133)
 puts("PASS actual tag133 full+trace START intersection and exact support identity; NO device");
#elif defined(PIONEER_Q38_QGW8_DENSE_STAGE_NATIVE_132)
 puts("PASS actual tag132 full+trace START intersection and exact support identity; NO device");
#else
 puts("PASS actual tag131 full+trace START intersection and exact support identity; NO device");
#endif
}
