/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_backend_role.h"
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTROL_BEGIN_FIXTURE
#include "pioneer_q38_token_native_control_begin_checks.inc"
#endif
static void CheckNativeBackendFirstMatrix(Fixture& f,uint64 epoch){
 auto& c=*f.c;auto& io=f.io;const auto learned=f.learned;
 const uint32_t prompt[]={248319,17},badPrompt[]={248320,17};
 const PioneerQ38TokenBackendResult pristine;
 const unsigned before=io.touches,pciEvents=io.nativePci.events;unsigned refusals=0;
 auto denied=[&](const PioneerQ38TokenBackendResult& result){
  assert(!memcmp(&result,&pristine,sizeof(result)) && io.touches==before && io.nativePci.events==pciEvents
   && !c.q38TokenNative && !c.q38Dispatch && sQ38TokenModelEpoch==epoch && f.learned==learned
   && !io.rootStarts && !io.executionCpu->steps && !io.bootstrapArms && !io.runtimePackets);
  refusals++;
 };
 auto begin=[&](PioneerProbeCookie* cookie,unsigned bank,const uint32_t* input,unsigned count,unsigned limit,unsigned eos0,unsigned eos1){
  return BeginQ38TokenNativeBackend(cookie,bank,input,count,limit,false,eos0,eos1);
 };
 denied(begin(&f.cookie,7,prompt,2,3,248318,248317));
 denied(begin(&f.cookie,UINT32_MAX,prompt,2,3,248318,248317));
 denied(begin(&f.cookie,0,nullptr,2,3,248318,248317));
 denied(begin(&f.cookie,0,prompt,0,3,248318,248317));
 denied(begin(&f.cookie,0,prompt,2,1,248318,248317));
 denied(begin(&f.cookie,0,prompt,2,0,248318,248317));
 denied(begin(&f.cookie,0,prompt,2,8193,248318,248317));
 denied(begin(&f.cookie,0,prompt,2,3,248320,248317));
 denied(begin(&f.cookie,0,prompt,2,3,248318,248320));
 denied(begin(&f.cookie,0,badPrompt,2,3,248318,248317));
 denied(begin(&f.cookie,0,reinterpret_cast<const uint32_t*>(reinterpret_cast<uintptr_t>(prompt)+1),2,3,248318,248317));
 denied(begin(&f.cookie,0,reinterpret_cast<const uint32_t*>(UINTPTR_MAX-3),2,3,248318,248317));
 auto rejectedCookie=[&](PioneerProbeCookie* cookie){
  denied(begin(cookie,0,prompt,2,3,248318,248317));denied(FinishQ38TokenNativeBackend(cookie));
 };
 rejectedCookie(nullptr);
 f.cookie.magic^=1;rejectedCookie(&f.cookie);f.cookie.magic^=1;
 currentTeam++;rejectedCookie(&f.cookie);currentTeam--;
 failThreadInfo=true;rejectedCookie(&f.cookie);failThreadInfo=false;
 currentThread=-1;rejectedCookie(&f.cookie);currentThread=17;
 lockStatus=B_BUSY;rejectedCookie(&f.cookie);lockStatus=B_OK;
 f.cookie.context=nullptr;rejectedCookie(&f.cookie);f.cookie.context=&c;
 sBankStorageOnly=false;rejectedCookie(&f.cookie);sBankStorageOnly=true;
 sRuntimeOwner=nullptr;rejectedCookie(&f.cookie);sRuntimeOwner=&f.cookie;
 denied(FinishQ38TokenNativeBackend(&f.cookie)); // never bound: NO cleanup authority
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  rejectedCookie(&f.cookie); // genuine nonreentrant OS lock, not a supplied phase
 }
 assert(!lockHeld && lockAcquires==lockReleases);
 printf("PASS private native backend %u argument/FD/team/thread/lock/unbound-END refusals preserve owners, epoch, all IO and outputs;NO device launch or cleanup grant\n",refusals);
 io.runtimeWorkspace=io.workspaceAddress;
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTROL_BEGIN_FIXTURE
 const auto result=CheckNativeControlBegin(f);
#else
 const auto result=begin(&f.cookie,0,prompt,2,3,248318,248317);
#endif
 if(!result.ready)fprintf(stderr,"BACKEND_BEGIN_DIAGNOSTIC stage%u started%u ready%u unsafe%u detail%x:%x:%x:%x;NO completion supplied\n",
  result.stage,result.started,result.ready,result.unsafe,result.detail[0],result.detail[1],result.detail[2],result.detail[3]);
 auto* state=c.q38TokenNative;
 assert(result.version==1 && result.started==1 && result.stage==8 && result.ready==1 && !result.unsafe && !result.complete
  && state && !state->Unsafe() && state->InitializationsClosed(c.banks,f.proof) && sQ38TokenModelEpoch==epoch+1
  && io.rootStarts==1010 && io.rootStops==1010 && io.rootHolds==1010
  && io.observedGdnPages==73728 && io.observedHistoryPages==1680 && io.observedProjectionPages==512
  && io.runtimePackets==4616 && io.bootstrapArms==119 && io.bootstrapStarts==119
  && io.executionArms==1 && io.executionStarts==1 && io.executionCpu->markerWrites==1
  && !io.executionCpu->submitted && !io.executionCpu->matrixInstructions && !io.executionCpu->floatOps
  && !state->TerminalObserved() && !state->TerminalHeld() && !state->Closed() && f.learned==learned);
 for(unsigned i=0;i<2;i++){
  const unsigned touches=io.touches;const auto again=begin(&f.cookie,0,prompt,2,3,248318,248317);
  const auto finish=FinishQ38TokenNativeBackend(&f.cookie);
  assert(!memcmp(&again,&pristine,sizeof(again)) && !memcmp(&finish,&pristine,sizeof(finish))
   && io.touches==touches && c.q38TokenNative==state && !state->Unsafe() && !state->Closed());
 }
 assert(!lockHeld && c.q38GdnCallThread==-1 && lockAcquires==lockReleases);
 puts("PASS actual composite Begin: SAME full851 owner -> 1010 real zero jobs -> native120 held -> runtime bytes ->119 virgin ISA boots -> genuine ROOT startup ACK;duplicate Begin and pending Finish refused without IO;NOT token/END/physical/Work/TPS");
 assert(CheckNativeTraffic(f) && CheckNativeEmbedding(f));CheckNativeLayerDispatch(f);CheckNativeFirstMatrix(f);
 const unsigned touches=io.touches;const auto finish=FinishQ38TokenNativeBackend(&f.cookie);
 assert(!memcmp(&finish,&pristine,sizeof(finish)) && io.touches==touches && !state->TerminalHeld() && !state->Unsafe());
 tt_resident_session_result retained={};bool clean=true;
 assert(!IncompleteEnd(c,retained,clean) && !clean && io.touches==touches);f.Unknown();
 puts("PASS composite Begin SAME original27B embedding/RMS/first matrix histories;pending Finish never fabricates STOP/drain/restore/END;resources retained;SIMULATED endpoints,NOT full layer/model/physical/TPS");
}
