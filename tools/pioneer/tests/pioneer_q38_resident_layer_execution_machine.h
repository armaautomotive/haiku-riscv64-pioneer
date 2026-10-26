/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_EXECUTION_MACHINE_H
#define PIONEER_Q38_RESIDENT_LAYER_EXECUTION_MACHINE_H
#include "pioneer_q38_projection_execution_machine.h"
// TEST BYTE endpoints only, never a source/clock/native/initialized grant.
struct ResidentLayerExecutionFabric:ProjectionExecutionFabric {
 virtual bool ReadState(unsigned bank,uint32_t address,uint32_t* out,unsigned words)=0;
 virtual bool WriteState(unsigned bank,uint32_t address,const uint32_t* input,unsigned words)=0;
};
class ResidentLayerExecutionMachine {
 struct Impl;Impl* implementation;
public:
 ResidentLayerExecutionMachine(const char* root,const char* peer,ResidentLayerExecutionFabric&);
 ~ResidentLayerExecutionMachine();
 void Start(unsigned w);void Advance(unsigned w);unsigned Pc(unsigned w)const;
 unsigned Boots()const;unsigned Stops()const;
 void VerifyOutput();void Diagnostic()const; // captured model state ONLY
 void StageFingerprints(uint64_t*)const; // actual emitted intermediate stores,TEST DATA ONLY
};
#endif
