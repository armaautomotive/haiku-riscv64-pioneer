/* SPDX-License-Identifier: MIT */
// TEST composition of actual native END/source bodies with raw OS/PCI endpoints.
 io.InitFinalVmViews();c.transaction.map_attempted=c.transaction.config_attempted=true;c.pci.version=1;c.pci.state=TT_PIONEER_MMIO_READ_OK;
 for(unsigned i=0;i<6;i++)c.pci.bars_before[i]=i%2?0:0x0c;c.pci.bridge_before[0]=0x1fff1;
#ifdef PIONEER_Q38_RECURRENT_BLOCK_FRESH_FINAL_TEST
 computeScope.reset();assert(!finalLockHeld && c.q38GdnCallThread==-1 && finalLockAcquired==1 && finalLockReleased==1);
 c.pci.node_id[0]=0x18611002; // explicit simulated connected PCIe node register DATA
 auto end=[&](tt_resident_session_result& r){return NativeSealedFinalEnd(&f.cookie,r);};
 for(unsigned bad=0;bad<11;bad++)RecurrentNativeFork([&]{
  if(bad==0)f.cookie.magic=0;if(bad==1)sRuntimeOwner=nullptr;if(bad==2)sBankStorageOnly=false;
  if(bad==3)c.bankStorageOnly=false;if(bad==4)io.q38MutableContext=nullptr;if(bad==5)c.q38GdnCallThread=42;
  if(bad==6)currentThread=-1;if(bad==7)currentTeam++;if(bad==8)failThreadInfo=true;if(bad==9)f.cookie.context=nullptr;
  auto clock=c.q38Clock.IO(),physical=transport.Touches();tt_resident_session_result closed={};
  auto invoke=[&]{return bad==10?NativeSealedFinalEnd(nullptr,closed):end(closed);};
  assert(invoke()!=B_OK && !finalLockHeld && !finalDestroyed && !finalUnreserved && !finalAuditCalls && !sColdRecoveryRequired);
  assert(invoke()!=B_OK && clock==c.q38Clock.IO() && physical==transport.Touches() && finalRemoved.empty() && !io.finalPciWrites);
 });
 for(unsigned bad=0;bad<7;bad++)RecurrentNativeFork([&]{
  if(bad==0)c.hashes=nullptr;if(bad==1)c.q38EmbeddingModel->token++;if(bad==2)sReloadGeneration++;
  if(bad==3)io.runtimeCore=119;if(bad==4)c.bytes-=4096;if(bad==5)c.runtimeCoreMode=0;if(bad==6)c.initialized=nullptr;
  auto clock=c.q38Clock.IO(),physical=transport.Touches();tt_resident_session_result closed={};end(closed);
  if(!(f.cookie.context==&c && c.dispatchOwnerRetained && sColdRecoveryRequired && !finalLockHeld && !finalDestroyed && !finalUnreserved && !finalAuditCalls && finalRemoved.empty() && !io.finalPciWrites))
   fprintf(stderr,"FRESH_BINDING_REFUSAL case%u context%u retained%u cold%u lock%u destroyed%u unreserved%u audit%u removed%zu pci%u clock%u/%u physical%u/%u\n",bad,unsigned(f.cookie.context==&c),unsigned(c.dispatchOwnerRetained),unsigned(sColdRecoveryRequired),unsigned(finalLockHeld),finalDestroyed,finalUnreserved,finalAuditCalls,finalRemoved.size(),io.finalPciWrites,clock,c.q38Clock.IO(),physical,transport.Touches());
  assert(f.cookie.context==&c && c.dispatchOwnerRetained && sColdRecoveryRequired && !finalLockHeld && !finalDestroyed && !finalUnreserved && !finalAuditCalls && finalRemoved.empty() && !io.finalPciWrites);
  assert(end(closed)!=B_OK && clock==c.q38Clock.IO() && physical==transport.Touches() && c.q38GdnCallThread==-1);
 });
 for(int kind=0;kind<12;kind++)RecurrentNativeFork([&]{
  finalAuditFailureKind=kind;finalAuditFailureOrdinal=kind==MixedFinalAuditEndpoints::Tlb?3:1;
  tt_resident_session_result closed={};end(closed);
  assert(f.cookie.context==&c && sRuntimeOwner==&f.cookie && sColdRecoveryRequired && c.dispatchOwnerRetained && !finalLockHeld && !finalDestroyed && !finalUnreserved && finalAuditCalls==1 && finalRemoved.empty() && !io.finalPciWrites);
  auto clock=c.q38Clock.IO(),physical=transport.Touches(),writes=finalAuditWrites;
  assert(end(closed)!=B_OK && clock==c.q38Clock.IO() && physical==transport.Touches() && finalAuditCalls==1 && finalAuditWrites==writes);
 });
 for(int fault=0;fault<5;fault++)RecurrentNativeFork([&]{
  if(fault<3)finalAuditDropRestoreWord=fault;if(fault==3)finalQuietFailure=true;if(fault==4)finalIdleFailure=true;
  tt_resident_session_result closed={};end(closed);
  assert(f.cookie.context==&c && sRuntimeOwner==&f.cookie && sColdRecoveryRequired && c.dispatchOwnerRetained && !finalLockHeld && !finalDestroyed && !finalUnreserved && finalAuditCalls==1 && finalRemoved.empty() && !io.finalPciWrites);
  auto clock=c.q38Clock.IO(),physical=transport.Touches(),writes=finalAuditWrites;
  assert(end(closed)!=B_OK && clock==c.q38Clock.IO() && physical==transport.Touches() && finalAuditCalls==1 && finalAuditWrites==writes);
 });
#else
 auto end=[&](tt_resident_session_result& r){NativeFinalTail(&f.cookie,r,false);return B_OK;};
 for(unsigned bad=0;bad<8;bad++)RecurrentNativeFork([&]{
  if(bad==0)currentTeam++;if(bad==1)sRuntimeOwner=nullptr;if(bad==2)io.q38MutableContext=nullptr;if(bad==3)c.hashes=nullptr;
  if(bad==4)c.q38EmbeddingModel->token++;if(bad==5)c.bankStorageOnly=false;if(bad==6)io.runtimeCore=119;if(bad==7)sReloadGeneration++;
  auto clockIO=c.q38Clock.IO(),physical=transport.Touches();tt_resident_session_result closed={};NativeFinalTail(&f.cookie,closed,false);
  assert(f.cookie.context==&c && c.dispatchOwnerRetained && sColdRecoveryRequired && !finalDestroyed && !finalUnreserved && finalRemoved.empty() && !io.finalPciWrites);
  NativeFinalTail(&f.cookie,closed,false);assert(clockIO==c.q38Clock.IO() && physical==transport.Touches() && finalRemoved.empty() && !io.finalPciWrites);
 });
#endif
 for(unsigned fault=0;fault<5;fault++)RecurrentNativeFork([&]{
  c.q38Clock.Fault(fault);tt_resident_session_result closed={};end(closed);
  assert(f.cookie.context==&c && sRuntimeOwner==&f.cookie && c.dispatchOwnerRetained && sColdRecoveryRequired && !finalDestroyed && !finalUnreserved && finalRemoved.empty() && !io.finalPciWrites);
  auto clockIO=c.q38Clock.IO();end(closed);assert(c.q38Clock.IO()==clockIO && finalRemoved.empty() && !io.finalPciWrites);
 });
 for(unsigned fail=1;fail<=33;fail++)RecurrentNativeFork([&]{
  finalDeleteFailure=fail;tt_resident_session_result closed={};
  end(closed);
  assert(f.cookie.context==&c && sRuntimeOwner==&f.cookie && !finalDestroyed && !finalUnreserved && c.dispatchOwnerRetained && sColdRecoveryRequired && finalDeleteFailed && finalRemoved.size()==fail);
  auto clockIO=c.q38Clock.IO();auto before=finalRemoved;auto writes=io.finalPciWrites;end(closed);
  assert(before==finalRemoved && writes==io.finalPciWrites && c.q38Clock.IO()==clockIO);
 });
 for(unsigned fail=1;fail<=11;fail++)RecurrentNativeFork([&]{
  finalPciFailure=fail;tt_resident_session_result closed={};
  end(closed);
  assert(f.cookie.context==&c && sRuntimeOwner==&f.cookie && !finalDestroyed && !finalUnreserved && c.dispatchOwnerRetained && sColdRecoveryRequired && finalPciFailed && finalRemoved.size()==30 && io.finalPciWrites==fail);
  auto clockIO=c.q38Clock.IO();auto reads=io.finalPciReads;end(closed);assert(reads==io.finalPciReads && finalRemoved.size()==30 && c.q38Clock.IO()==clockIO);
 });
 assert(f.c.release()==&c);tt_resident_session_result closed={};assert(end(closed)==B_OK);
 assert(!f.cookie.context && !sRuntimeOwner && finalDestroyed==1 && finalUnreserved==1 && io.q38RecurrentBlockFinalUnmapped && finalRemoved.size()==33 && io.finalPciWrites==11 && closed.state==TT_RESIDENT_SESSION_CLOSED && closed.host_released && closed.pci_restored && !closed.cold);
#ifdef PIONEER_Q38_RECURRENT_BLOCK_FRESH_FINAL_TEST
 assert(finalAuditCalls==1 && finalAuditWrites && !finalLockHeld && finalLockAcquired==2 && finalLockReleased==2);
 for(unsigned kind=0;kind<12;kind++)assert(finalAuditReads[kind]);
 puts("PASS GENUINE private native closed + ACTUAL compute-call scope release -> ACTUAL locked bank-END seal/owned prefix+footer/real clock/fresh VERBATIM7-parent AfterNodeId+PCI quiet+resident idle/recheckTRUE/final scope/30VM+11PCI+3host release/unreserve+cookie-global clear+recurrent-prefix destruction;84cases=11early+7binding+12audit+3audit-restore+quiet+idle+5clock+33VMhost+11PCI fence later IO/disposal/retry;SIMULATED OS/registers,NO original-model physical/full64/Work/TPS qualification");
#else
 puts("PASS GENUINE native closed -> VERBATIM actual owned END prefix+footer/bulk-root closure/real private clock/final scope/30VM Unmap/11PCI writes+proof/actual3host release methods/unreserve+cookie-global clear+recurrent-prefix destruction;8binding+5clock+33VM-host+11PCI refusals retain context/reservation and fence later disposal/retry;SIMULATED OS/PCI,NO fresh diagnostic recheck/complete callseal/physical qualification");
#endif
