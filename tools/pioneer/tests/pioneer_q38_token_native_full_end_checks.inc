/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_token_native_full_end_role.h"
#ifdef PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE
#include "pioneer_q38_token_native_state_snapshot.inc"
#endif
// Distinct positive committed-token lifetime. Never reuse a cancellation,
// zero-record,old processor or supplied clean/ready proof to reach END.
static void CheckNativeFullEndPending(Fixture& f){
 auto& c=*f.c;auto& io=f.io;const unsigned before=io.touches,pci=io.nativePci.events,arc=c.q38ClockIO.rawEvents;
 unsigned refused=0;
 auto probe=[&](auto& result,auto factory){
  const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
  for(unsigned i=0;i<16;i++){words[i]^=1;assert(!factory(result));result=pristine;refused++;}
  assert(!factory(result) && !memcmp(&result,&pristine,sizeof(result)));refused++;
 };
 auto stages=[&](){
  PioneerQ38TokenRestoreResult l1;probe(l1,[&](auto& r){return RestoreQ38TokenNativeL1(c,r);});
  PioneerQ38TokenDispatchRestoreResult dispatch;probe(dispatch,[&](auto& r){return RestoreQ38TokenNativeDispatchImages(c,r);});
  PioneerQ38TokenPeerRestoreResult peers;probe(peers,[&](auto& r){return RestoreQ38TokenNativePeerStores(c,r);});
  PioneerQ38TokenRootStoreRestoreResult root;probe(root,[&](auto& r){return RestoreQ38TokenNativeRootStore(c,r);});
  PioneerQ38TokenSharedTlbRestoreResult shared;probe(shared,[&](auto& r){return RestoreQ38TokenNativeSharedMappings(c,r);});
  PioneerQ38TokenClockResult clock;probe(clock,[&](auto& r){return EndQ38TokenNativeClock(c,r);});
  PioneerQ38TokenHardwareSealResult hardware;probe(hardware,[&](auto& r){return SealQ38TokenNativeHardware(c,r);});
  PioneerQ38TokenViewsResult views;probe(views,[&](auto& r){return CloseQ38TokenNativeViews(c,r);});
  PioneerQ38TokenPciResult config;probe(config,[&](auto& r){return CloseQ38TokenNativePci(c,r);});
  PioneerQ38TokenHostsResult hosts;probe(hosts,[&](auto& r){return CloseQ38TokenNativeHosts(c,r);});
  for(unsigned kind:{0U,1U,2U,3U,UINT32_MAX}){assert(io.TokenHostReleaseChecked(kind)==B_NOT_ALLOWED);refused++;}
  assert(io.TokenUnreserveDeviceChecked()==B_NOT_ALLOWED);refused++;
 };
 stages();
 {PioneerQ38TokenCallScope invalid(nullptr);PioneerQ38TokenEndResult end;probe(end,[&](auto& r){return invalid.Finish(r);});}
 {PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);stages();
  PioneerQ38TokenEndResult end;probe(end,[&](auto& r){return scope.Finish(r);});}
 assert(refused==386 && io.touches==before && io.nativePci.events==pci && c.q38ClockIO.rawEvents==arc && !io.rootStarts
  && !io.nativeUnreserveCalls && io.nativeViewDeletes.empty() && io.nativeHostDeletes.empty() && !c.q38TokenNative->Unsafe()
  && !c.q38TokenNative->Closed() && !io.nativeTrace->Embeddings());
 printf("PASS separate complete-generation cleanup TEST/actual bound owner+real simulated clock/PCI BEGIN/%u nonpristine,pending,outside-FD and host/unreserve refusals/ZERO following model or clock/PCI IO,instructions,VM-host release or unreserve;NO supplied terminal/ready/full END/physical/Work/TPS proof\n",refused);
}
static void CheckNativeFullEndFlow(Fixture& f){
 namespace L=PioneerQ38TokenL1;
 auto& c=*f.c;auto& io=f.io;auto* s=c.q38TokenNative;auto& cpu=*io.executionCpu;
 assert(!lockHeld && !io.nativeEndObserved && s->TerminalHeld() && !s->Closed() && !s->Unsafe()
  && io.nativeTrace->SavedComplete() && cpu.pc==0x60004 && cpu.flowLayers==192 && cpu.flowPosition==2
  && !io.executionCancelWrites && io.kvWords.size()==49152 && io.kvWriteWords==49152
  && io.beforeNativeImage.size()==L::SavedWords && io.beforeDispatchImage.size()==120U*(TT_Q38_PANEL_VIEW_WORDS+1U)
  && io.beforePeerScratch.size()==119U*2048 && io.nativeHostDeletes.empty() && !io.nativeUnreserveCalls);
 io.nativeEndObserved=true;const unsigned packets=io.runtimePackets,rootPackets=cpu.layerPackets;
 const uint64 instructions=cpu.ObservedRootInstructions();
#ifdef PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE
 Q38NativeStateTest snapshots;assert(tokenNativeStateSnapshotDirectory);
 // Reconstruct public fixed GEOMETRY for this const DATA observer. Do not
 // expose or friend the private native owner,or use a new layout as a
 // source/bank/launch/terminal/cleanup grant.
 auto snapshotLayout=std::make_unique<PioneerQ38CanonicalModelPlan>();
 assert(snapshotLayout->Build() && CaptureNativeStateSnapshots(io,*snapshotLayout,snapshots,tokenNativeStateSnapshotDirectory));
#endif
 // No pending output may be replaced by cleanup. The actual 2+1 readout
 // calls have drained the committed frontier before this first restore.
 auto step=[&](auto& result,auto factory,auto predicate){
  const auto pristine=result;const unsigned before=io.touches,pci=io.nativePci.events,arc=c.q38ClockIO.rawEvents;
  assert(!factory(result) && !memcmp(&result,&pristine,sizeof(result)) && io.touches==before && io.nativePci.events==pci && c.q38ClockIO.rawEvents==arc);
  {PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);auto* words=reinterpret_cast<uint32*>(&result);
   for(unsigned i=0;i<16;i++){words[i]^=1;assert(!factory(result) && io.touches==before && io.nativePci.events==pci && c.q38ClockIO.rawEvents==arc);result=pristine;}
   assert(factory(result) && result.ready && !result.unsafe && predicate() && !s->Unsafe() && !s->Closed());
   auto again=pristine;assert(!factory(again));}
 };
 PioneerQ38TokenRestoreResult l1;io.nativeRestoreResult=&l1;
 step(l1,[&](auto& r){return RestoreQ38TokenNativeL1(c,r);},[&](){return s->OriginalL1Restored();});io.nativeRestoreResult=nullptr;
 assert(l1.restoredWords==L::SavedWords && l1.verifiedWords==L::SavedWords);
 for(unsigned w=0;w<120;w++)for(unsigned i=0;i<L::SnapshotWords(w);i++)assert(io.Raw(w,L::SnapshotAddress(w,i))==io.beforeNativeImage[L::SnapshotBase(w)+i]);
 PioneerQ38TokenDispatchRestoreResult dispatch;io.nativeDispatchResult=&dispatch;
 step(dispatch,[&](auto& r){return RestoreQ38TokenNativeDispatchImages(c,r);},[&](){return s->DispatchImagesRestored();});io.nativeDispatchResult=nullptr;
 for(unsigned w=0;w<120;w++){
  const unsigned base=w*(TT_Q38_PANEL_VIEW_WORDS+1U);assert(io.Raw(w,0x6000)==io.beforeDispatchImage[base]);
  for(unsigned i=0;i<TT_Q38_PANEL_VIEW_WORDS;i++)assert(io.Raw(w,TT_Q38_PANEL_VIEW_BASE+i*4)==io.beforeDispatchImage[base+1+i]);
 }
 PioneerQ38TokenPeerRestoreResult peers;io.nativePeerResult=&peers;
 step(peers,[&](auto& r){return RestoreQ38TokenNativePeerStores(c,r);},[&](){return s->PeerStoreOriginalsRestored();});io.nativePeerResult=nullptr;
 for(unsigned w=1;w<120;w++){
  assert(io.Core(w).config==io.beforePeerConfig[w-1]);
  for(unsigned i=0;i<2048;i++)assert(io.Raw(w,0x17e000+i*4)==io.beforePeerScratch[(w-1)*2048+i]);
 }
 PioneerQ38TokenRootStoreRestoreResult root;io.nativeRootStoreResult=&root;
 step(root,[&](auto& r){return RestoreQ38TokenNativeRootStore(c,r);},[&](){return s->RootStoreOriginalsRestored();});io.nativeRootStoreResult=nullptr;
 assert(io.Core(0).config==io.beforeRootConfig);for(unsigned i=0;i<2048;i++)assert(io.Raw(0,0x17e000+i*4)==io.beforeRootScratch[i]);
 PioneerQ38TokenSharedTlbRestoreResult shared;io.nativeSharedTlbResult=&shared;
 step(shared,[&](auto& r){return RestoreQ38TokenNativeSharedMappings(c,r);},[&](){return s->SharedMappingsRestored();});io.nativeSharedTlbResult=nullptr;
 assert(shared.restoredWords==6 && shared.verifiedWords==6 && io.sharedAliasRefusals==9);
 PioneerQ38TokenClockResult clock;step(clock,[&](auto& r){return EndQ38TokenNativeClock(c,r);},[&](){return s->ClockRestored();});
 assert(clock.beforeMHz==800 && clock.afterMHz==800 && clock.messages==2 && clock.tlbWords==6);
 PioneerQ38TokenHardwareSealResult hardware;io.nativeHardwareSealResult=&hardware;
 step(hardware,[&](auto& r){return SealQ38TokenNativeHardware(c,r);},[&](){return s->HardwareLeasesSealed();});io.nativeHardwareSealResult=nullptr;
 PioneerQ38TokenViewsResult views;step(views,[&](auto& r){return CloseQ38TokenNativeViews(c,r);},[&](){return s->ViewsReleased();});assert(views.deleted==29);
 PioneerQ38TokenPciResult pci;step(pci,[&](auto& r){return CloseQ38TokenNativePci(c,r);},[&](){return s->PciRestored();});assert(pci.writes==11);
 PioneerQ38TokenHostsResult hosts;step(hosts,[&](auto& r){return CloseQ38TokenNativeHosts(c,r);},[&](){return s->HostBuffersReleased();});assert(hosts.released==3);
 assert(io.runtimePackets==packets && cpu.layerPackets==rootPackets && cpu.ObservedRootInstructions()==instructions
  && io.nativeViewDeletes.size()==29 && io.nativePci.writes==22 && !io.HostReadMock::bmeWrites);
 {PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);PioneerQ38TokenEndResult end;const auto pristine=end;
  auto* words=reinterpret_cast<uint32*>(&end);for(unsigned i=0;i<16;i++){words[i]^=1;assert(!scope.Finish(end) && !io.nativeUnreserveCalls);end=pristine;}
  assert(scope.Finish(end) && end.ready && !end.unsafe && end.context && end.reservation && end.hosts==3 && !end.unreserve_status);
  (void)f.c.release(); // Finish actually deleted c+state;do NOT inspect their stale local references
  assert(!scope.Context() && !f.cookie.context && !sRuntimeOwner && !sBankStorageOnly && !io.q38MutableContext && io.nativeUnreserveCalls==1);
  PioneerQ38TokenEndResult again;assert(!scope.Finish(again) && !again.started);}
 assert(!lockHeld && lockAcquires==lockReleases && !f.c && !f.cookie.context && !sColdRecoveryRequired);
#ifdef PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE
 assert(snapshots.Finish(tokenNativeStateSnapshotDirectory));
 puts("OBSERVED native176 state records/GDN slots after2+3/Conv after3/packed half K+V all3positions/SAME actual physical TEST arrays;marker saved AFTER genuine full END;independent CPU comparison STILL REQUIRED,not hardware/TPS qualification");
#endif
 puts("EXECUTED native SAME genuine3tokens*64layers+positive committed readout/all original L1+dispatch+peer+ROOT+shared restorations/normal clock END/hardware+29VM+PCI closure/3host releases/checked unreserve+locked whole-context disposal;NO prior cancellation/empty-readout/history transplant or CPU products supplied;independent full numeric/physical/Work/TPS qualification STILL REQUIRED");
}
