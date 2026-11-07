/* SPDX-License-Identifier: MIT */
// Non-loadable kernel startup serializer witness; no native owner or IO.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38TokenStartupPlan.h"
extern "C" bool qgw8_startup_build(PioneerQ38TokenStartupPlan& startup,const PioneerQ38CanonicalModelPlan& plan,
 unsigned bank,const PioneerQ38GdnIdentity& id,const uint32_t* sources,const uint32_t* prompt){
 return startup.Build(plan,bank,id,sources,prompt,1,8192,true,248318,248317);
}
extern "C" bool qgw8_startup_head(const PioneerQ38TokenStartupPlan& startup,uint8_t* out){
 return startup.ReadHead(0,out,512);
}
