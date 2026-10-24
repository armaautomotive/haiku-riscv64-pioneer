/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_NEXT_PROJECTION_EXECUTION_MACHINE_H
#define PIONEER_Q38_NEXT_PROJECTION_EXECUTION_MACHINE_H
#include "pioneer_q38_projection_execution_machine.h"
// TEST byte-only fabric:actual QPS2 plus unchanged QFP3 instructions.
// No native producer/ready/epoch construction and no synthetic learned fallback.
class NextProjectionExecutionMachine {
 struct Impl;Impl* implementation;
public:
 NextProjectionExecutionMachine(const char*,const char*,ProjectionExecutionFabric&);
 ~NextProjectionExecutionMachine();
 static bool GrammarSelfTest(); // separate-TU ODR check BEFORE costly model/context setup
 void Start(unsigned);void Advance(unsigned);unsigned Pc(unsigned)const;
 unsigned Matrices()const;unsigned Producers()const;unsigned Stops()const;
 void VerifyOutput();void Diagnostic()const;
};
#endif
