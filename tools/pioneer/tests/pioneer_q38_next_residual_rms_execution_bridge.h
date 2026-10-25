/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_RESIDUAL_RMS_EXECUTION_BRIDGE_H
#define PIONEER_Q38_NEXT_RESIDUAL_RMS_EXECUTION_BRIDGE_H
#include "pioneer_q38_projection_execution_machine.h"
class PioneerQ38CanonicalModelPlan;
// TEST byte endpoints only. Actual installed QRR2 instructions produce
// outputs;no completed producer/owner/ledger or ready flag is manufactured.
unsigned ExecuteNextResidualRmsNativeBytes(const char*,const PioneerQ38CanonicalModelPlan&,ProjectionExecutionFabric&,bool badWeight=false);
#endif
