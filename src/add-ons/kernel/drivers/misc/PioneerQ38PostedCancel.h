/* SPDX-License-Identifier: MIT */
// Fixed RAW32 cancellation transport, not a STOP/cleanup or Work capability.
#ifdef _KERNEL_MODE
#if !defined(PIONEER_Q38_TOKEN_POSTED_CANCEL) || PIONEER_Q38_TOKEN_POSTED_CANCEL != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS) || PIONEER_Q38_TOKEN_FAULT_PROGRAMS != 1 \
 || defined(PIONEER_Q38_POSTED_CANCEL_TEST)
#error Separate G3 fault-ROM native posted-cancel selection required
#endif
#include "PioneerQ38TokenNativeRole.h"
#include "../../../../../tools/pioneer/PioneerQ38TokenFaultProgramsRole.h"
#else
#if !defined(PIONEER_Q38_POSTED_CANCEL_TEST) || PIONEER_Q38_POSTED_CANCEL_TEST != 1 \
 || defined(PIONEER_Q38_TOKEN_POSTED_CANCEL) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE)
#error Explicit HOST transport TEST only;no native or Work capability
#endif
#endif
#ifndef PIONEER_Q38_POSTED_CANCEL_H
#define PIONEER_Q38_POSTED_CANCEL_H
#include "PioneerComputeFleet.h"
#include "../../../../../tools/pioneer/PioneerQ38TokenLayout.h"

class PioneerQ38PostedCancel {
public:
 enum Phase {Pristine,Checking,Switching,Posting,Delivering,Restoring,Complete,Unknown};
 enum {PollLimit=4096};
private:
 Phase phase=Pristine;
 bool attempted=false,changed=false;
 bool Fail(){phase=Unknown;return false;} // metadata ONLY;never retry/restore after uncertainty
 template<class IO> bool Scope(IO& io)const{return !Failed() && io.PostedCancelScope(*this);}
 template<class IO> bool Maps(IO& io,bool posted)const{
  for(unsigned slot=0;slot<2;slot++)for(unsigned word=0;word<3;word++){
   uint32_t value=0;
   if(!Scope(io) || !io.PostedCancelTlbRead(slot,word,value) || value!=Expected(slot,word,posted))return false;
  }
  return Scope(io);
 }
public:
 PioneerQ38PostedCancel()=default;
 PioneerQ38PostedCancel(const PioneerQ38PostedCancel&)=delete;
 PioneerQ38PostedCancel& operator=(const PioneerQ38PostedCancel&)=delete;
 Phase Current()const{return phase;}
 bool Active()const{return attempted && phase!=Complete;}
 bool Failed()const{return phase==Unknown;}
 bool Changed()const{return changed;}
 bool Delivered()const{return attempted && phase==Complete && !changed;}
 static uint32_t Expected(unsigned slot,unsigned word,bool posted){
  if(slot>=2 || word>=3)return UINT32_MAX;
  uint32_t value=BlackholeDramFixedTlbWord(slot?0xffa00000U:0U,PioneerWorkerX(0),PioneerWorkerY(0),false,word);
  // Only TLB200's ordering bits70..71 change:Strict AXI(1) -> Posted(2).
  // Coordinates,NoC0,base,unicast and TLB201 remain EXACTLY the same.
  return posted && !slot && word==2?(value&~0xc0U)|0x80U:value;
 }
 template<class IO> bool Request(IO& io){
  if(attempted || phase!=Pristine || !Scope(io))return false;
  attempted=true;phase=Checking;
  if(!Maps(io,false) || !Scope(io) || !io.PostedCancelSourceGuard())return Fail();
  uint32_t value=0;
  if(!Scope(io) || !io.PostedCancelWordRead(value) || value)return Fail();
  const uint64_t began=io.PostedCancelNow();uint64_t previous=began;
  if(began>UINT64_MAX-UINT64_C(30000000))return Fail();
  phase=Switching;changed=true; // mark BEFORE the first actual mode write
  if(!Scope(io) || !io.PostedCancelControlWrite(true) || !Maps(io,true)
   || !Scope(io) || !io.PostedCancelSourceGuard())return Fail();
  phase=Posting;
  if(!Scope(io) || !io.PostedCancelWordWrite())return Fail(); // EXACTLY ONE write
  phase=Delivering;bool delivered=false;
  for(unsigned poll=0;poll<PollLimit;poll++){
   if(!Scope(io))return Fail();
   const uint64_t now=io.PostedCancelNow();
   if(now<previous || now<began || now-began>=UINT64_C(30000000))return Fail();
   previous=now;
   if(!Maps(io,true) || !Scope(io) || !io.PostedCancelWordRead(value))return Fail();
   if(value==PioneerQ38TokenLayout::CancelMagic){delivered=true;break;}
   if(value || !Scope(io))return Fail();
   io.PostedCancelPause();
  }
  if(!delivered || !Scope(io) || !io.PostedCancelSourceGuard())return Fail();
  // Posted PCIe/NoC acknowledgment alone is NOT delivery. Only the actual
  // destination readback above permits this successful normal-mode return.
  phase=Restoring;
  if(!Scope(io) || !io.PostedCancelControlWrite(false) || !Maps(io,false)
   || !Scope(io) || !io.PostedCancelSourceGuard()
   || !Scope(io) || !io.PostedCancelWordRead(value) || value!=PioneerQ38TokenLayout::CancelMagic || !Scope(io))return Fail();
  changed=false;phase=Complete;return true; // request delivered ONLY,not a STOP receipt
 }
};
#endif
