/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_OUTPUT_EXECUTION_MACHINE_H
#define PIONEER_Q38_GDN_OUTPUT_EXECUTION_MACHINE_H
#include "pioneer_q38_projection_execution_machine.h"
// TEST instruction machines only. Real native scope/claims/bootstrap/boot
// come from the actual controller. This interface grants NONE of them.
class GdnOutputExecutionMachine {
 struct Impl;Impl* implementation;
public:
 GdnOutputExecutionMachine(const char*,const char*,ProjectionExecutionFabric&);
 ~GdnOutputExecutionMachine();
 void Start(unsigned);void Advance(unsigned);unsigned Pc(unsigned)const;
 unsigned Matrices()const;unsigned Producers()const;unsigned Stops()const;
 void VerifyOutput();void Diagnostic()const;
};
#endif
