/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_backend_init_fault_role.h"
static void CheckNativeBackendInitFault(Fixture& f,unsigned fault,uint64 epoch){
 auto& c=*f.c;auto& io=f.io;assert(fault<4 && !c.q38TokenNative && !lockHeld);
 const auto learned=f.learned;const unsigned pci=io.nativePci.events,arc=c.q38ClockIO.rawEvents;
 if(fault==0)io.failRun=1; // corrupt one actually executed zero-ROM summary
 if(fault==1)io.loseTeamRun=1;
 if(fault==2)io.loseGenerationRun=1;
 if(fault==3)io.noCompletion=true; // genuine simulated timeout, no completion supplied
 const uint32_t prompt[]={248319,17};
 const auto result=BeginQ38TokenNativeBackend(&f.cookie,0,prompt,2,3,false,248318,248317);
 assert(result.started==1 && result.stage==2 && result.unsafe==1 && !result.ready && !result.complete
  && result.detail[0]==1 && c.q38TokenNative && c.q38TokenNative->Unsafe() && !c.q38TokenNative->Closed()
  && !c.q38TokenNative->RecurrentInitialized(c.banks,f.proof) && sQ38TokenModelEpoch==epoch+1
  && io.rootStarts==1 && !io.rootStops && !io.rootHolds && !c.q38Dispatch && !io.q38TokenMemoryMapAttempted
  && !io.runtimePackets && !io.bootstrapArms && !io.bootstrapStarts && !io.executionArms && !io.executionStarts
  && !io.executionCpu->steps && f.learned==learned && io.nativePci.events==pci && c.q38ClockIO.rawEvents==arc
  && io.nativeViewDeletes.empty() && io.nativeHostDeletes.empty() && !io.nativeUnreserveCalls);
 for(const auto& peer:io.bootstrapPeer)assert(!peer->steps && !peer->deviceWrites);
 const unsigned touches=io.touches;const PioneerQ38TokenBackendResult pristine;
 const auto again=BeginQ38TokenNativeBackend(&f.cookie,0,prompt,2,3,false,248318,248317);
 const auto end=FinishQ38TokenNativeBackend(&f.cookie);
 assert(!memcmp(&again,&pristine,sizeof(again)) && !memcmp(&end,&pristine,sizeof(end))
  && io.touches==touches && io.nativePci.events==pci && c.q38ClockIO.rawEvents==arc);
 f.Unknown();assert(io.touches==touches && !lockHeld && lockAcquires==lockReleases);
 printf("PASS composite Begin early-init fault%u/stage2/one actual zero request then strict failure/ALL owners quarantined/no publication,worker bootstrap,ROOT/model start,following IO,automatic rollback,END,free or retry;TEST ledger/OS/MMIO only,NOT original-model authentication/positive initialization/physical/Work/TPS qualification\n",fault);
}
