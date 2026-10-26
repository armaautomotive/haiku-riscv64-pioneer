/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_RUN_OBSERVATION_H
#define PIONEER_Q38_RESIDENT_LAYER_RUN_OBSERVATION_H
#include <stdint.h>
// CPU-only failure evidence, never a completion/owner/IO/cleanup grant.
// Callers supply values from EXISTING reads only. The first failure freezes
// the record; inspecting it must never read or repair the device.
struct PioneerQ38ResidentLayerRunObservation {
 enum Failure : uint32_t {
  None,Scope,LearnedPages,InitialGuard,InitialBudget,ArmGuard,PollGuard,
  PollBudget,Marker,Receipt,PcOutsideImage,PcRejected,PollsExhausted,
  PeerBudget,PeerReceipt,FinalGuard,FinalPhysical,FinalBudget,FinalHold
 };
 uint32_t version=2;
 Failure failure=None;
 uint32_t polls=0,status[3]={},statusValid=0,statusReads=0;
 uint32_t pc[2]={},pcReads=0,reset=0,resetReads=0;
 uint32_t unexpectedPcCount=0,outsideImageCount=0,rejectedPcCount=0,firstUnexpectedPc=0,firstUnexpectedPoll=0;
 void Poll(){if(failure==None)polls++;}
 void Status(unsigned index,uint32_t value){
  if(failure!=None || index>=3)return;
  status[index]=value;statusValid|=1U<<index;statusReads++;
 }
 void Pc(uint32_t value){if(failure==None){pc[0]=pc[1];pc[1]=value;pcReads++;}}
 void RunningPc(uint32_t value,bool immutable,bool rejected){
  if(failure!=None)return;
  Pc(value);
  if(immutable && !rejected)return;
  if(!unexpectedPcCount){firstUnexpectedPc=value;firstUnexpectedPoll=polls;}
  unexpectedPcCount++;outsideImageCount+=!immutable;rejectedPcCount+=rejected;
 } // frontend observations ONLY;NO execution/completion or failure authority
 void Reset(uint32_t value){if(failure==None){reset=value;resetReads++;}}
 void Fail(Failure first){if(failure==None && first!=None)failure=first;}
};
static_assert(sizeof(PioneerQ38ResidentLayerRunObservation)==72,"CPU observation only;not public ABI or readiness state");
#endif
