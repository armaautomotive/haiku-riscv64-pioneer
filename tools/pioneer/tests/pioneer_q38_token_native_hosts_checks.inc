/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_HOSTS_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HOSTS_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_VIEWS_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_VIEWS_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_HARDWARE_SEAL_FIXTURE != 1 \
 || !defined(PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_CLOCK_FIXTURE != 1 \
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
#error Explicit native full-predecessor host-release and locked-END TEST required
#endif
static void CheckNativeHostsClose(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* s=c.q38TokenNative;
 assert(!io.nativeEndObserved);io.nativeEndObserved=true;
 assert(!lockHeld && s->PciRestored() && !s->HostBuffersReleased() && !s->Closed() && io.nativeHostDeletes.empty() && !io.nativeUnreserveCalls);
 const unsigned pci=io.nativePci.events,arc=c.q38ClockIO.rawEvents,packets=io.runtimePackets,before=io.touches;
 PioneerQ38TokenHostsResult result;const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
 PioneerQ38TokenEndResult end;const auto pristineEnd=end;auto* endWords=reinterpret_cast<uint32*>(&end);
 assert(!CloseQ38TokenNativeHosts(c,result) && io.nativeHostDeletes.empty());
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  assert(!scope.Finish(end) && !end.started && !io.nativeUnreserveCalls);
  for(unsigned i=0;i<16;i++){words[i]^=1;assert(!CloseQ38TokenNativeHosts(c,result) && io.nativeHostDeletes.empty());result=pristine;}
  const bool okay=CloseQ38TokenNativeHosts(c,result);
  if(io.nativeHostFault){
   const unsigned released=io.nativeHostFaultAt-(io.nativeHostFault==1);
   assert(!okay && result.started && result.unsafe && !result.ready && result.released==released && s->Unsafe() && !s->Closed()
    && io.nativeHostDeletes.size()==io.nativeHostFaultAt && !io.nativeUnreserveCalls && c.dispatchOwnerRetained && c.banks.Quarantined());
   for(unsigned i=0;i<3;i++)assert(io.TokenHostView(i)==(i>=3-released?-1:int32_t(301+i)));
   PioneerQ38TokenHostsResult again;assert(!CloseQ38TokenNativeHosts(c,again) && !scope.Finish(end));
  }else{
   if(!okay)fprintf(stderr,"HOST_CLOSE_DIAGNOSTIC started%u unsafe%u released%u handles%d/%d/%d;NO caller release receipt supplied\n",
    result.started,result.unsafe,result.released,io.TokenHostView(0),io.TokenHostView(1),io.TokenHostView(2));
   assert(okay && result.ready && !result.unsafe && result.released==3 && s->HostBuffersReleased() && !s->Closed()
    && !s->Unsafe() && io.TokenHostReleaseScope() && (io.nativeHostDeletes==std::vector<unsigned>{2,1,0})
    && c.host.released && c.bulkHost.released && c.wideHost.released && !c.bulkHost.ready && !c.wideHost.ready);
   for(unsigned kind:{0U,1U,2U,3U,UINT32_MAX})assert(io.TokenHostReleaseChecked(kind)==B_NOT_ALLOWED);
   const auto calls=io.nativeHostDeletes.size();PioneerQ38TokenHostsResult again;assert(!CloseQ38TokenNativeHosts(c,again) && io.nativeHostDeletes.size()==calls);
   auto refuse=[&](auto& field,auto bad){const auto saved=field;field=bad;assert(!scope.Finish(end) && !end.started && !io.nativeUnreserveCalls);field=saved;};
   refuse(currentThread,99);refuse(currentTeam,99);refuse(sReloadGeneration,sReloadGeneration+1);
   refuse(c.Pci().restored,0U);refuse(c.bulkHost.pending,true);refuse(c.wideHost.pending,true);refuse(io.inputArea,301);
   for(unsigned i=0;i<16;i++){endWords[i]^=1;assert(!scope.Finish(end) && !io.nativeUnreserveCalls);end=pristineEnd;}
   auto* original=f.c.get();const bool finished=scope.Finish(end);
   if(io.nativeUnreserveFail || io.nativeUnreserveLose){
    assert(!finished && end.started && end.unsafe && !end.ready && !end.context && io.nativeUnreserveCalls==1
     && end.reservation==unsigned(!io.nativeUnreserveFail) && end.unreserve_status==(io.nativeUnreserveFail?B_DEV_NOT_READY:B_OK)
     && s->Unsafe() && !s->Closed() && f.cookie.context==original && f.c.get()==original && c.dispatchOwnerRetained);
    PioneerQ38TokenEndResult retry;assert(!scope.Finish(retry) && io.TokenUnreserveDeviceChecked()==B_NOT_ALLOWED && io.nativeUnreserveCalls==1);
   }else{
    assert(finished && end.started && end.ready && !end.unsafe && end.hosts==3 && end.reservation && end.context && !end.unreserve_status
     && !scope.Context() && !f.cookie.context && !sRuntimeOwner && !sBankStorageOnly && !io.q38MutableContext && io.nativeUnreserveCalls==1);
    assert(f.c.get()==original);(void)f.c.release(); // actual production Finish already deleted the fixture context
    PioneerQ38TokenEndResult retry;assert(!scope.Finish(retry) && !retry.started); // no UAF or second unreserve
   }
  }
 }
 const unsigned endpointTouches=io.nativeHostFault==1 && io.nativeHostFaultAt>1?2:1;
 assert(!lockHeld && lockAcquires==lockReleases && io.nativePci.events==pci && io.runtimePackets==packets && io.touches==before+endpointTouches);
 // The clock object belonged to the deleted context;do not touch it again.
 if(f.c)assert(f.c->q38ClockIO.rawEvents==arc);
 if(io.nativeHostFault || io.nativeUnreserveFail || io.nativeUnreserveLose)
  puts("EXECUTED native SAME actual full predecessor/host release or unreserve uncertainty/known successful prefix recorded/remaining context quarantined/NO later release,unreserve,PCI,MMIO,retry or false full END;SIMULATED OS endpoints,NOT physical/Work/TPS qualification");
 else puts("EXECUTED native SAME actual full predecessor through PCI/3 exact OS host releases/checked same-cookie device unreserve/locked whole-context destruction/sticky bank+model+CPU journals retired/scope unlock without dangling dereference/NO later PCI,MMIO,OS release or retry;SIMULATED endpoints,NOT public ABI/Assistant/hardware/Work/TPS qualification");
}
