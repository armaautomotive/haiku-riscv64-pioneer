/* SPDX-License-Identifier: MIT */
// Fixed observation DATA template compile only;no native readback or IO.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerReadbackPlan.h"
extern "C" bool pioneer_layer_readback_plan_compile(PioneerQ38ResidentLayerReadbackPlan& plan,
 const PioneerQ38ResidentLayerPlan& layer,unsigned page,PioneerQ38ResidentLayerReadbackPage& output){
 return plan.Build(layer) && plan.Page(page,output);
}
