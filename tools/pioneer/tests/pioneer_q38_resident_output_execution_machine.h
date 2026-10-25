/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_OUTPUT_EXECUTION_MACHINE_H
#define PIONEER_Q38_RESIDENT_OUTPUT_EXECUTION_MACHINE_H
#include "pioneer_q38_projection_execution_machine.h"
// TEST BYTE endpoints only, never a source/clock/native/initialized grant.
struct ResidentOutputExecutionFabric:ProjectionExecutionFabric {
 virtual bool ReadState(unsigned bank,uint32_t address,uint32_t* out,unsigned words)=0;
 virtual bool WriteState(unsigned bank,uint32_t address,const uint32_t* input,unsigned words)=0;
};
class ResidentOutputExecutionMachine {
 struct Impl;Impl* implementation;
public:
 ResidentOutputExecutionMachine(const char* root,const char* peer,ResidentOutputExecutionFabric&);
 ~ResidentOutputExecutionMachine();
 void Start(unsigned w);void Advance(unsigned w);unsigned Pc(unsigned w)const;
 unsigned Boots()const;unsigned Stops()const;
 void VerifyOutput();void Diagnostic()const; // captured model state ONLY
};
#endif
