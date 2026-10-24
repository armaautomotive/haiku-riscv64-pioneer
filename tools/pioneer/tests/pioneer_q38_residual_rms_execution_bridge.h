/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDUAL_RMS_EXECUTION_BRIDGE_H
#define PIONEER_Q38_RESIDUAL_RMS_EXECUTION_BRIDGE_H
#include "pioneer_q38_projection_execution_machine.h"
class PioneerQ38CanonicalModelPlan;
// TEST byte fabric only. No owner/readiness/counter/numeric result is
// supplied to the controller;actual installed instructions make outputs.
unsigned ExecuteResidualRmsNativeBytes(const char* image,const PioneerQ38CanonicalModelPlan&,ProjectionExecutionFabric&,bool badWeight=false);
#endif
