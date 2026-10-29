/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_SHARED_TLB_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_ROOT_STORE_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_PEERS_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_DISPATCH_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_RESTORE_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_READOUT_EMPTY_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HOLD_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CANCEL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_FULL_LAYER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PEER_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) || PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY != 1 \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE)
#error Explicit native shared-map-preceded clock-END TEST required
#endif
static void CheckNativeClockEnd(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* state=c.q38TokenNative;auto& clock=c.q38ClockIO;
 assert(!lockHeld && state->SharedMappingsRestored() && !state->ClockRestored() && !state->Closed()
  && c.q38Clock.RequiresExplicitEnd() && clock.notifications==3 && clock.clock==1350 && !clock.deletes);
 const unsigned rawBefore=clock.rawEvents,before=io.touches,packets=io.runtimePackets;
 std::array<unsigned,120> resets;for(unsigned w=0;w<120;w++)resets[w]=io.Core(w).resets.size();
 PioneerQ38TokenClockResult result;const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
 assert(!EndQ38TokenNativeClock(c,result) && clock.rawEvents==rawBefore && io.touches==before);
 struct Loss{Fixture* fixture;PioneerQ38TokenClockResult* result;unsigned seen=0,raw=0,model=0;} loss{&f,&result};
 clock.observer=&loss;clock.after=[](void* p,unsigned kind,unsigned){
  auto& s=*static_cast<Loss*>(p);auto& c=*s.fixture->c;
  if(c.tokenClockFault==1 && !s.seen && s.result->started && s.result->phase==1 && kind==2){
   s.seen++;sReloadGeneration++;s.raw=c.q38ClockIO.rawEvents;s.model=s.fixture->io.touches;
  }
 }; // generation loss AFTER an actual ARC-window write;no successful receipt supplied
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned i=0;i<16;i++){
   words[i]^=1;assert(!EndQ38TokenNativeClock(c,result) && clock.rawEvents==rawBefore && io.touches==before);result=pristine;
  }
  if(c.tokenClockFault==2)clock.badReply=true;
  if(c.tokenClockFault==3)clock.failDelete=1;
  const bool okay=EndQ38TokenNativeClock(c,result);
  if(c.tokenClockFault){
   assert(!okay && result.started && result.unsafe && !result.ready && state->Unsafe() && !state->ClockRestored()
    && c.q38Clock.Unsafe() && !state->Closed());
   if(c.tokenClockFault==1)assert(loss.seen==1 && clock.tokenClockGuard.Stopped() && clock.rawEvents==loss.raw
    && io.touches==loss.model && !clock.deletes && c.q38Clock.RequiresExplicitEnd());
   if(c.tokenClockFault==2)assert(clock.notifications==4 && !clock.deletes && c.q38Clock.RequiresExplicitEnd());
   if(c.tokenClockFault==3)assert(clock.notifications==5 && clock.deletes==1 && c.q38Clock.RequiresExplicitEnd());
  }else{
   if(!okay)fprintf(stderr,"NATIVE_CLOCK_END_DIAGNOSTIC phase%u unsafe%u raw%u notifications%u deletes%u state%u required%u stopped%u;NO caller clock success supplied\n",
    result.phase,result.unsafe,clock.rawEvents,clock.notifications,clock.deletes,c.q38Clock.Snapshot().state,
    unsigned(c.q38Clock.RequiresExplicitEnd()),unsigned(clock.tokenClockGuard.Stopped()));
   assert(okay && result.ready && !result.unsafe && result.messages==2 && result.beforeMHz==800 && result.afterMHz==800 && result.tlbWords==6
    && state->ClockRestored() && !state->SharedMappingsRestored() && !state->Closed() && !state->Unsafe()
    && !c.q38Clock.RequiresExplicitEnd() && !c.q38Clock.Unsafe() && clock.notifications==5 && clock.clock==800 && clock.deletes==6
    && clock.tokenClockGuard.Sealed() && !clock.tokenClockGuard.Stopped() && clock.TokenClockViewsReleased()
    && io.TokenModelSourceScope(false) && !io.TokenMemoryAccessScope());
   for(unsigned s=0;s<2;s++)for(unsigned i=0;i<3;i++)assert(io.Core(0).tlb[s][i]==io.beforeSharedTlb[s][i]);
  }
  const unsigned raw=clock.rawEvents,touches=io.touches;PioneerQ38TokenClockResult again;
  assert(!EndQ38TokenNativeClock(c,again) && clock.rawEvents==raw && io.touches==touches);
 }
 clock.observer=nullptr;clock.after=nullptr;
 for(unsigned w=0;w<120;w++)assert(io.Core(w).coreReset==0x47800 && io.Core(w).resets.size()==resets[w]);
 assert(c.lease.ready && c.bulk.ready && !c.bulk.dirty && !c.q38Dispatch->closed && io.runtimePackets==packets
  && !io.pciWrites && !io.HostReadMock::bmeWrites && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
 if(c.tokenClockFault)printf("EXECUTED native clock-END fault%u/actual SAME original firstlayer+consumed cancel/STOP/HOLD/drain/token+dispatch+119peer+ROOT+shared originals/real clock lease/first transport-generation-or-response-or-view-deletion uncertainty/ALL remaining owners retained/no retry/NO full END/physical/Work/TPS grant\n",c.tokenClockFault);
 else puts("EXECUTED native SAME full851/original firstlayer+cancel/STOP/HOLD/drain/token+dispatch+119peer+ROOT+shared originals/real clock BEGIN800->1350 and private native per-access-scoped END1350->800/2 real simulated ARC messages/6 clock-only VM deletions/6 shared-original control readbacks/sealed transport;master/bulk/modelVM/hosts/reservations STILL OWNED,NO full END/physical/Work/TPS grant");
}
