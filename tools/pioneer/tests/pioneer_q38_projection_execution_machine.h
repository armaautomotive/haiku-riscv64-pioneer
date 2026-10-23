/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_EXECUTION_MACHINE_H
#define PIONEER_Q38_PROJECTION_EXECUTION_MACHINE_H
#include <stdint.h>
#include <vector>
// TEST transport interface,not production/native device authority.
struct ProjectionExecutionFabric {
 virtual ~ProjectionExecutionFabric()=default;
 virtual uint32_t ReadLocal(unsigned w,uint32_t a)=0;
 virtual void WriteLocal(unsigned w,uint32_t a,uint32_t v)=0;
 virtual uint32_t NoC(unsigned w,unsigned offset)=0;
 virtual void SetControls(unsigned w,const uint32_t* command,const uint32_t* counts)=0;
 virtual uint32_t Reset(unsigned w)=0;
 virtual std::vector<uint32_t>& Gddr()=0;
};
class ProjectionExecutionMachine {
 struct Impl;Impl* implementation;
public:
 ProjectionExecutionMachine(const char* root,const char* peer,ProjectionExecutionFabric&);
 ~ProjectionExecutionMachine();
 void Start(unsigned w);void Advance(unsigned w);unsigned Pc(unsigned w)const;
 unsigned Matrices()const;unsigned Producers()const;unsigned Stops()const;
 void VerifyOutput();
 void Diagnostic()const; // captured CPU model state ONLY;zero endpoint reads
};
#endif
