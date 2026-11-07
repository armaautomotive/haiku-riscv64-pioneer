/* SPDX-License-Identifier: MIT */
// Compile-only kernel ABI witness; not a driver and no ownership/IO.
#define PIONEER_Q38_QGW8_LAYOUT_INSPECTION 1
#define PIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY 1
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Qgw8KernelLayout.h"
extern "C" bool qgw8_kernel_layout_build(PioneerQ38Qgw8KernelLayout& layout,const PioneerQ38CanonicalModelPlan& source){
 return layout.Build(source);
}
