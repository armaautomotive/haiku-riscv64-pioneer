/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE) || PIONEER_Q38_TOKEN_NATIVE_PCI_FIXTURE != 1 \
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
#error Explicit native VM-closed PCI restoration TEST required
#endif
static void CheckNativePciClose(Fixture& f){
 auto& c=*f.c;auto& io=f.io;auto* s=c.q38TokenNative;auto& raw=io.nativePci;
 assert(!lockHeld && s->ViewsReleased() && !s->PciRestored() && !s->Closed() && raw.writes==11 && raw.mmioReads==3
  && c.transaction.config_attempted && c.transaction.map_attempted);
 const unsigned events=raw.events,before=io.touches,arc=c.q38ClockIO.rawEvents,packets=io.runtimePackets;
 PioneerQ38TokenPciResult result;const auto pristine=result;auto* words=reinterpret_cast<uint32*>(&result);
 io.nativePciResult=&result;
 raw.observer=&io;raw.after=[](void* p,bool root,unsigned offset,unsigned width,bool write){
  auto& io=*static_cast<PioneerSessionIO*>(p);
  if(io.nativePciFault==1 && !io.nativePciFaultSeen && io.nativePciResult && io.nativePciResult->started
   && write && !root && offset==4 && width==2){io.nativePciFaultSeen++;io.nativePciFaultEvents=io.nativePci.events;sReloadGeneration++;}
 };
 assert(!CloseQ38TokenNativePci(c,result) && raw.events==events);
 {
  PioneerQ38TokenCallScope scope(&f.cookie);assert(scope.Context()==&c);
  for(unsigned i=0;i<16;i++){words[i]^=1;assert(!CloseQ38TokenNativePci(c,result) && raw.events==events);result=pristine;}
  const bool okay=CloseQ38TokenNativePci(c,result);
  if(io.nativePciFault){
   assert(!okay && result.started && result.unsafe && !result.ready && result.writes==1 && raw.writes==12
    && io.nativePciFaultSeen==1 && raw.events==io.nativePciFaultEvents && s->Unsafe() && !s->PciRestored()
    && !c.Pci().restored && c.Pci().state==TT_PIONEER_MMIO_RESTORE_FAILED && !c.transaction.config_attempted && !c.transaction.map_attempted);
  }else{
   if(!okay)fprintf(stderr,"PCI_CLOSE_DIAGNOSTIC started%u unsafe%u writes%u reads%u events%u;NO caller restore receipt supplied\n",
    result.started,result.unsafe,result.writes,result.reads,raw.events-events);
   assert(okay && result.started && result.ready && !result.unsafe && result.writes==11 && result.reads==72
    && raw.writes==22 && raw.events-events==83 && s->PciRestored() && !s->Closed() && !s->Unsafe() && io.TokenPciScope()
    && c.Pci().restored && c.Pci().stage==5 && !c.transaction.config_attempted && !c.transaction.map_attempted);
   for(unsigned i=0;i<6;i++)assert(raw.bars[i]==c.Pci().bars_before[i] && c.Pci().bars_after[i]==raw.bars[i]);
   for(unsigned i=0;i<3;i++)assert(raw.bridge[i]==c.Pci().bridge_before[i] && c.Pci().bridge_after[i]==raw.bridge[i]);
   assert(!raw.command[0] && !raw.command[1]);
   const unsigned done=raw.events;uint32 value=0x3751a9c2;
   for(bool root:{false,true})assert(!io.TokenPciReadChecked(root,0,4,value) && value==0x3751a9c2 && !io.TokenPciWriteChecked(root,4,2,0));
   auto refuse=[&](auto& field,auto bad){const auto saved=field;field=bad;assert(!io.TokenPciScope() && raw.events==done);field=saved;};
   refuse(currentThread,99);refuse(currentTeam,99);refuse(sReloadGeneration,sReloadGeneration+1);
   refuse(c.Pci().restored,0U);refuse(c.Pci().bars_after[5],1U);refuse(c.profile.card_bus,4U);
   refuse(c.bulkHost.pending,true);refuse(c.wideHost.pending,true);
   assert(io.TokenPciScope() && raw.events==done);
  }
  const unsigned done=raw.events;PioneerQ38TokenPciResult again;
  assert(!CloseQ38TokenNativePci(c,again) && raw.events==done);
 }
 raw.observer=nullptr;raw.after=nullptr;io.nativePciResult=nullptr;
 assert(io.touches==before && c.q38ClockIO.rawEvents==arc && io.runtimePackets==packets && io.nativeViewDeletes.size()==29
  && raw.mmioReads==3 && !io.wideReleased && !io.PageMock::releases && !io.OutputBufferMock::releases);
 if(io.nativePciFault)printf("EXECUTED native PCI close fault%u/actual full predecessor+real BEGIN+native prelaunch capture+hardware+VM closure/first actual config-write generation loss or readback corruption/NO following IO/retry/host release;transaction consumed/remaining owners retained,NOT full END/physical/Work/TPS\n",io.nativePciFault);
 else puts("EXECUTED native SAME full predecessor+prelaunch SAME actual PCI BEGIN+transport journal+hardware+29VM closure/72 checked config reads+11 exact ordered original writes+full final pass/both decoders OFF/8 metadata refusals/no post-close PCI traffic/no retry;hosts+model reservations STILL OWNED,NOT full END/physical/Work/TPS qualification");
}
