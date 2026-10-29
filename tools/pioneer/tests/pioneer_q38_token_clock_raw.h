/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_CLOCK_RAW_TEST_H
#define PIONEER_Q38_TOKEN_CLOCK_RAW_TEST_H
#if !defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE)
#error Explicit native clock transport TEST role required
#endif
// TEST raw ARC/VM endpoints only. The actual extracted adapter and actual
// ClockLease/normal-clock/strict-queue bodies execute on this backing.
#define PIONEER_Q38_ARC_QUEUE_MODEL_ONLY
#include "pioneer_q38_arc_queue_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ClockLease.h"
#ifndef PIONEER_PCI_PROFILE_H
struct tt_pioneer_profile {};
#endif
struct PioneerArcQueryIO;
static PioneerArcQueryIO* tokenClockRaw=nullptr;
struct PioneerArcQueryIO:StrictQueue {
 area_id queueWriteArea=-1,msiArea=-1,csmArea=-1,scratchArea=-1,tlbArea=-1,area=-1;
 volatile uint32* queueWrite=nullptr;volatile uint32* msi=nullptr;volatile uint32* tlb=nullptr;
 const volatile uint32* csm=nullptr;const volatile uint32* scratch=nullptr;const volatile uint32* nodeID=nullptr;
 unsigned maps=0,deletes=0,failDelete=0,hardwareCleanup=0,rawEvents=0;
 bool failMap=false;uint32* sharedWords=nullptr;
 void* observer=nullptr;void (*after)(void*,unsigned,unsigned)=nullptr;
 PioneerArcQueryIO(const tt_pioneer_profile&,tt_arc_scratch_result&,tt_arc_telemetry_result&,tt_arc_queue_result&){tokenClockRaw=this;}
 void Event(unsigned kind,unsigned argument=0){rawEvents++;if(after)after(observer,kind,argument);}
 bool Map(){
  maps++;if(failMap)return false;
  queueWriteArea=101;msiArea=102;csmArea=103;scratchArea=104;tlbArea=105;area=106;
  queueWrite=msi=MockQueue::csm.data();csm=scratch=nodeID=MockQueue::csm.data();tlb=MockQueue::tlb;return true;
 }
 uint32 TlbRead(unsigned word){assert(word<3);io++;const uint32 value=sharedWords?sharedWords[word]:MockQueue::tlb[word];Event(1,word);return value;}
 void TlbWrite(unsigned word,uint32 value){
  StrictQueue::TlbWrite(word,value);if(sharedWords && tlbWrites!=dropTlb)sharedWords[word]=value;Event(2,word);
 } // one actual raw control write;the simulated CSM window observes the same selected route
 uint32 CsmRead(uint32 address){const uint32 value=StrictQueue::CsmRead(address);Event(3,address);return value;}
 void SetQueueBounds(uint32 base,uint32 request,uint32 response){StrictQueue::SetQueueBounds(base,request,response);Event(4,base);}
 void QueueWrite(uint32 address,uint32 value){StrictQueue::QueueWrite(address,value);Event(5,address);}
 uint32 MsiStatusRead(){const uint32 value=StrictQueue::MsiStatusRead();Event(6);return value;}
 void NotifyArc(){StrictQueue::NotifyArc();Event(7);}
 void Pause(){StrictQueue::Pause();Event(8);}
 bool Unmap(){hardwareCleanup++;return false;} // forbidden inherited diagnostic cleanup
};
static int (*tokenModelViewDelete)(area_id)=nullptr; // OS endpoint only,never a native phase/clean receipt
static int delete_area(area_id view){
 if((view<101 || view>106) && tokenModelViewDelete)return tokenModelViewDelete(view);
 assert(tokenClockRaw && view>=101 && view<=106);auto& raw=*tokenClockRaw;raw.deletes++;raw.Event(9,unsigned(view));
 return raw.failDelete==raw.deletes?-1:B_OK;
}
#include "token_native_clock_io.inc"
#endif
