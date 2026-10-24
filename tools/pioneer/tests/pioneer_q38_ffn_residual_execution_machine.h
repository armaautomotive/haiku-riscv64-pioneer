/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FFN_RESIDUAL_EXECUTION_MACHINE_H
#define PIONEER_Q38_FFN_RESIDUAL_EXECUTION_MACHINE_H
#include "pioneer_q38_projection_execution_machine.h"
// TEST ONLY actual installed RV32/FPU/Tensix+byte endpoints,not a native
// controller/source/ready/producer capability. No numeric CPU input route.
class FfnResidualExecutionMachine {
 struct Impl;Impl* implementation;
public:
 FfnResidualExecutionMachine(const char* root,const char* peer,ProjectionExecutionFabric&);
 ~FfnResidualExecutionMachine();
 void Start(unsigned worker);void Advance(unsigned worker);unsigned Pc(unsigned worker)const;
 unsigned Matrices()const;unsigned Producers()const;unsigned Stops()const;
 void VerifyOutput();void Diagnostic()const;
 void FaultLearnedTile();bool FaultInjected()const; // TEST corrupt packet payload,not authenticated source cache
};
#endif
