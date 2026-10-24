/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_RMS_EXECUTION_MACHINE_H
#define PIONEER_Q38_NEXT_RMS_EXECUTION_MACHINE_H
#include "pioneer_q38_projection_execution_machine.h"
class PioneerQ38CanonicalModelPlan;
// TEST exact installed QRN1 byte execution,not an owner or ready grant.
class NextRmsExecutionMachine {
 struct Impl;Impl* implementation;
public:
 NextRmsExecutionMachine(const char*,const PioneerQ38CanonicalModelPlan&,ProjectionExecutionFabric&,bool badWeight=false);
 ~NextRmsExecutionMachine();
 void Advance();unsigned Pc()const;unsigned Packets()const;bool FaultInjected()const;void Verify();
};
#endif
