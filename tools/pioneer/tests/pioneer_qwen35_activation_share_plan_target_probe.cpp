/* SPDX-License-Identifier: MIT */
// Type-check actual model-recording planner on the Haiku host ABI; no IO.
#include "../PioneerQwen35DataflowActivationSharePlan.h"
using namespace PioneerQwen35;
extern "C" bool q35_share_plan_add(DataflowActivationSharePlan* plan,const DataflowCoreInvocation* invocation) {
    return plan && invocation && plan->Add(*invocation);
}
extern "C" bool q35_share_plan_seal(DataflowActivationSharePlan* plan) {
    return plan && plan->Seal();
}
extern "C" bool q35_share_plan_get(DataflowActivationSharePlan* plan,unsigned core,DataflowActivationShareMembership* output) {
    return plan && output && plan->Get(core,*output);
}
