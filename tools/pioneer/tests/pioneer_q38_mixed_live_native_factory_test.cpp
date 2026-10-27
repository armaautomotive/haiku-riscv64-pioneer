/* SPDX-License-Identifier: MIT */
// Genuine native factories/scopes/held journal/complete Dispatch END.
// Only OS/PCI/L1/time endpoints are simulated; NO native phase/ready setters.
#include <cstdint>
#include "../PioneerQ38MixedLiveBlockRootLayout.h"
#define PIONEER_Q38_EMBEDDING_FACTORY_FIXTURE_ONLY
#include "pioneer_q38_embedding_native_factory_test.cpp"
#include <sys/wait.h>
#include <unistd.h>
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchMmioEnd.h"
static unsigned finalUnreserved=0;
static struct{unsigned bus=1,device=2,function=0;}sCard;
static int sCookie=0;
#define TT_PCI_PROBE_DEVICE "misc/tenstorrent_pci_probe"
struct FinalPciService{
 void unreserve_device(unsigned b,unsigned d,unsigned fn,const char* name,int* cookie){
  assert(b==1 && d==2 && !fn && !strcmp(name,TT_PCI_PROBE_DEVICE) && cookie==&sCookie && finalRemoved.size()==33 && !finalDeleteFailed && !finalPciFailed);finalUnreserved++;
  auto* c=sRuntimeOwner->context;assert(c && c->q38Clock.Snapshot().after_mhz==800 && c->q38Clock.Notifications()==5 && c->q38Clock.Unmaps()==1);
 }
};
static FinalPciService finalPciService;static FinalPciService* sPCI=&finalPciService;
static void NativeFinalTail(PioneerProbeCookie* cookie,tt_resident_session_result& result,bool recheck){
 auto* context=cookie->context;bool clean=!sColdRecoveryRequired && !context->Unsafe();
#include "native_final_prefix.inc"
 assert(false && "the existing native mixed source must refuse any non-bank fallback");return;
#include "native_final_tail.inc"
// Actual source footer closes this function itself.
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38DispatchMmioEnd.h"
struct NativeClockPciFailureEndpoint{
 PioneerStoreContext& c;unsigned attempts=0;
 bool Unmap(){
  attempts++;assert(!PioneerQ38MixedLiveFinalPciUncertain(c) && !c.Unsafe() && !c.pci.restored);
  c.io.q38MixedLiveFinalUnmapAttempted=true;return false; // simulated first VM deletion failure ONLY
 }
 void CardWrite(unsigned,unsigned,uint32){assert(false && "no PCI write after Unmap failure");}
 void RootWrite(unsigned,unsigned,uint32){assert(false && "no PCI write after Unmap failure");}
 uint32 CardRead(unsigned,unsigned){assert(false && "no PCI read after Unmap failure");return 0;}
 uint32 RootRead(unsigned,unsigned){assert(false && "no PCI read after Unmap failure");return 0;}
};
#endif
template<class Check> static void NativeFactoryFork(Check check){
 fflush(nullptr);pid_t pid=fork();assert(pid>=0);if(!pid){check();_exit(0);}
 int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
}
int main(int argc,char** argv){
 assert(argc==7 || (argc==8 && (!strcmp(argv[7],"--execute") || !strcmp(argv[7],"--faults") || !strcmp(argv[7],"--validation-readback") || !strcmp(argv[7],"--clock-end") || !strcmp(argv[7],"--final-end"))));setvbuf(stdout,nullptr,_IOLBF,0);
 bool readback=argc==8 && !strcmp(argv[7],"--validation-readback");
 bool execute=readback || (argc==8 && !strcmp(argv[7],"--execute")),faults=argc==8 && !strcmp(argv[7],"--faults");unsigned refused=0;
#ifndef PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK
 assert(!readback);
#endif
#ifndef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
 assert(argc!=8 || strcmp(argv[7],"--clock-end")); // a held-only binary must not impersonate the clock test
#endif
#ifndef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
 assert(argc!=8 || strcmp(argv[7],"--final-end"));
#endif
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;
 (void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 NativeFactoryFixture f(argv[1],argv[2]);auto& c=*f.c;auto& io=f.io;
 auto loseScope=[&](unsigned bad){
  if(bad==0)io.q38MutableContext=nullptr;if(bad==1)sRuntimeOwner=nullptr;
#ifdef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
  if(bad==2)c.q38Clock.Quarantine();
#else
  if(bad==2)c.q38Clock.value.busy_mhz=0;
#endif
  if(bad==3)c.q38EmbeddingModel->token++;if(bad==4)c.hashes=nullptr;if(bad==5)currentTeam++;
  if(bad==6)f.transfer.ready=false;if(bad==7)c.span.closed=false;
 };
#ifndef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
 c.q38Clock.value.busy_mhz=1400;
#endif
 auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 for(unsigned i=0;i<plan->Count();i++){
  const auto* p=plan->Part(i);if(p->kind!=plan->WeightKind || p->layer!=3)continue;
  unsigned first=p->bank*unsigned(plan->Capacity/4096)+p->offset/4096;
  for(unsigned j=0;j<p->allocated/4096;j++)f.learned[first+j]=1;
 }
 assert(BindQ38EmbeddingModel(c,f.token,0,f.bank) && f.Initialize() && f.Run());
 assert(c.q38EmbeddingModel->Closed() && io.MixedLiveBaseScope());
 MixedLiveNativeTransport transport(argv[3],argv[4],argv[5],argv[6]);
 // Copy the actual post-QPE1 root bytes/controls. No completed producer,
 // model, ledger, bank, context, identity or retained lease is manufactured.
 std::vector<uint32> addresses,values;auto seed=[&](uint32 a,uint32 v){addresses.push_back(a);values.push_back(v);};
 seed(0x6000,io.entry);
 for(unsigned i=0;i<io.prefix.size();i++)seed(0x64000+i*4,io.prefix[i]);
 for(unsigned i=0;i<io.view.size();i++)seed(TT_Q38_PANEL_VIEW_BASE+i*4,io.view[i]);
 for(unsigned i=0;i<2048;i++)seed(0x17e000+i*4,io.destination[i]);
 uint32 source[16],command[10],counts[8];
 for(unsigned i=0;i<16;i++)source[i]=i?io.source[i]:io.config;
 for(unsigned i=0;i<10;i++)command[i]=io.command[i];
 for(unsigned i=0;i<8;i++)counts[i]=io.PanelIO::ResidentNocRead(PioneerQ38PanelCounter(i));
 transport.SeedRoot(addresses.data(),values.data(),unsigned(addresses.size()),source,command,counts);
 auto before=transport.Fingerprint();auto learned=f.learned;
 io.mixedLiveTransport=&transport;
 assert(BindQ38MixedLiveNative(c));auto* state=c.q38MixedLiveNative;
 assert(state && !state->Closed() && !state->Unsafe() && io.MixedLiveBaseScope());
 PioneerQ38MixedLiveMemoryResult result;
 if(faults){
  for(unsigned word=0;word<12;word++){
   PioneerQ38MixedLiveMemoryResult bad;reinterpret_cast<uint32*>(&bad)[word]^=1;
   auto physical=transport.Touches(),touches=io.touches;
   assert(!PrepareQ38MixedLiveNativeHeld(c,bad) && !c.q38Dispatch && io.touches==touches && transport.Touches()==physical
    && io.MixedLiveBaseScope() && !state->Unsafe());refused++;
  }
  for(unsigned null=0;null<2;null++)for(unsigned map=0;map<4;map++)NativeFactoryFork([&]{
   if(null)io.mixedLiveNullMap=int(map);else io.mixedLiveFailMap=int(map);
   PioneerQ38MixedLiveMemoryResult bad;assert(!PrepareQ38MixedLiveNativeHeld(c,bad));
   assert(io.mixedLiveMapFaultFence && io.touches==io.mixedLiveMapFaultFence && state->Unsafe() && c.dispatchOwnerRetained
    && c.q38Dispatch && c.q38Dispatch->unsafe && c.q38Clock.quarantined && f.transfer.quarantined && f.wide.quarantined
    && c.banks.Quarantined() && sColdRecoveryRequired && !transport.Resets());
   for(unsigned i=0;i<map;i++)assert(io.q38MixedLiveMemoryArea[i]>=B_OK && io.q38MixedLiveMemoryView[i]);
   auto physical=transport.Touches(),touches=io.touches;
   assert(!PrepareQ38MixedLiveNativeHeld(c,bad) && !CloseQ38MixedLiveNativeHeld(c,bad) && !EndFactory(c)
    && io.touches==touches && transport.Touches()==physical);
  });
  refused+=8;
 }
 bool prepared=PrepareQ38MixedLiveNativeHeld(c,result);
 if(!prepared)fprintf(stderr,"NATIVE_HELD_PREPARE_FAILURE prepared%u routes%u phase%u worker%u word%u unsafe%u touches%u\n",
  c.q38Dispatch?c.q38Dispatch->prepared:0,c.q38DispatchRoutes,result.phase,result.worker,result.word,state->Unsafe(),transport.Touches());
 assert(prepared && c.q38Dispatch && c.q38Dispatch->prepared==120 && c.q38DispatchRoutes && !state->Closed() && !state->Unsafe());
 assert(result.savedWords==TT_Q38_MIXED_LIVE_SAVED_WORDS && result.installedWords==TT_Q38_MIXED_LIVE_CODE_WORDS
  && result.verifiedOriginalWords==TT_Q38_MIXED_LIVE_SAVED_WORDS && io.mapCalls==5 && !transport.Resets());
 puts("PASS actual native held factories/ALL120 genuine probes+peer leases+role preparation/native scopes+four mappings/full original journal+code install;NO ready facade or reset");
 if(faults){
  for(unsigned word=0;word<16;word++){
   PioneerQ38MixedLiveResult bad;reinterpret_cast<uint32*>(&bad)[word]^=1;
   auto physical=transport.Touches(),touches=io.touches;
   assert(!PrepareQ38MixedLiveNativeExecution(c,bad) && io.touches==touches && transport.Touches()==physical
    && io.MixedLiveBaseScope() && !state->Unsafe());refused++;
  }
  for(unsigned bad=0;bad<8;bad++)NativeFactoryFork([&]{
   loseScope(bad);
   auto physical=transport.Touches(),touches=io.touches;
   assert(!CloseQ38MixedLiveNativeHeld(c,result) && state->Unsafe() && c.dispatchOwnerRetained
    && c.q38Dispatch->unsafe && c.q38Clock.quarantined && f.transfer.quarantined && f.wide.quarantined && sColdRecoveryRequired);
   assert(!CloseQ38MixedLiveNativeHeld(c,result) && !EndFactory(c) && io.touches==touches && transport.Touches()==physical && !transport.Resets());
  });
  refused+=8;
 }
 PioneerQ38MixedLiveResult execution;
 if(execute){
  io.ConfigureMixedLiveTestTransport();transport.Observe(execution);
  bool ready=PrepareQ38MixedLiveNativeExecution(c,execution);
  if(!ready){fprintf(stderr,"NATIVE_EXECUTION_PREPARE_FAILURE phase%u worker%u word%u\n",execution.phase,execution.worker,execution.word);transport.Diagnostic();}
  assert(ready && !state->Closed() && !state->Unsafe());
  puts("PASS actual native execution preparation/released119 real fresh boots/actual private execution scope/ROOT held;NO synthetic ready");
  bool ran=RunQ38MixedLiveNativeExecution(c,execution);
  if(!ran){fprintf(stderr,"NATIVE_EXECUTION_RUN_FAILURE phase%u worker%u word%u\n",execution.phase,execution.worker,execution.word);transport.Diagnostic();}
  assert(ran);transport.VerifyRun();
  for(unsigned bad=0;bad<8;bad++)NativeFactoryFork([&]{
   loseScope(bad);auto physical=transport.Touches(),touches=io.touches,resets=transport.Resets();
   assert(!CloseQ38MixedLiveNativeExecution(c,execution,result) && state->Unsafe() && c.dispatchOwnerRetained
    && c.q38Dispatch->unsafe && c.q38Clock.quarantined && f.transfer.quarantined && f.wide.quarantined && sColdRecoveryRequired
    && !execution.stopped && !execution.held && !result.restoredWords);
   assert(!CloseQ38MixedLiveNativeExecution(c,execution,result) && !EndFactory(c)
    && transport.Touches()==physical && io.touches==touches && transport.Resets()==resets);
  });
  puts("PASS actual native post-RUN lifetime refusals8/real completed block and still-running SAME119 retained/ALLhosts+clock quarantined/ZERO following MMIO,STOP,hold,restore or retry");
  bool closed=false;
#ifdef PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK
  if(readback){
   std::vector<uint32_t> values(5120,0xdeadbeef);PioneerQ38MixedLiveReadbackResult capture;
   // Native CPU-invalid requests MUST NOT consume ExecutionComplete or
   // quarantine an otherwise valid owner. No physical endpoint is touched.
   auto touches=io.touches,physical=transport.Touches();
   assert(!CloseQ38MixedLiveNativeValidation(c,execution,result,values.data(),5119,capture) && !state->Unsafe()
    && touches==io.touches && physical==transport.Touches() && !capture.started);
   closed=CloseQ38MixedLiveNativeValidation(c,execution,result,values.data(),unsigned(values.size()),capture);
   assert(closed && capture.completed && capture.pages==5 && capture.words==5120);transport.VerifyReadback(values.data(),unsigned(values.size()));
   puts("PASS actual native fixed5page/5120word validation readback from REAL instruction-produced GDDR AFTER119STOP+ROOT/ALL120holds;atomic output after full restore+secondproof;NO CPU expected products supplied");
  }else
#endif
  closed=CloseQ38MixedLiveNativeExecution(c,execution,result);
  if(!closed){fprintf(stderr,"NATIVE_EXECUTION_CLOSE_FAILURE phase%u worker%u word%u\n",execution.phase,execution.worker,execution.word);transport.Diagnostic();}
  assert(closed && execution.completed==119 && execution.stopped==119 && execution.held==120);transport.VerifyClosed();
 }else assert(CloseQ38MixedLiveNativeHeld(c,result) && !transport.Resets());
 assert(state->Closed() && !state->Unsafe());
 assert(result.restoredWords==TT_Q38_MIXED_LIVE_SAVED_WORDS && result.verifiedOriginalWords==2*TT_Q38_MIXED_LIVE_SAVED_WORDS);
 if(faults){
  for(unsigned bad=0;bad<4;bad++)NativeFactoryFork([&]{
   if(bad==0)c.q38EmbeddingModel->token++;if(bad==1)c.hashes=nullptr;if(bad==2)currentTeam++;if(bad==3)io.runtimeCore=119;
   auto physical=transport.Touches(),touches=io.touches;
   assert(!EndFactory(c) && state->Unsafe() && c.dispatchOwnerRetained && !c.q38Dispatch->closed
    && !EndFactory(c) && io.touches==touches && transport.Touches()==physical);
  });
  refused+=4;
 }
 assert(EndFactory(c) && c.q38Dispatch->closed && !c.dispatchOwnerRetained && !sColdRecoveryRequired);
 assert(transport.Fingerprint()==before && f.learned==learned && !io.pciWrites && !io.HostReadMock::bmeWrites);
 for(unsigned w=1;w<120;w++)assert(!c.q38Dispatch->worker[w].Store().ready && c.q38Dispatch->worker[w].Store().niu_restored
  && c.q38Dispatch->worker[w].Store().tlb_restored);
 printf("PASS actual native %s/full11398712word restore+secondproof/complete sealed Dispatch END/legacy roles+peer leases restored/learned unchanged;SIMULATED endpoints/NOT physical hardware,full64model,Work or TPS\n",execute?"RUN+real119STOP+ALL120terminals before holds":"HELD no-launch close");
 if(faults)printf("PASS actual native factory refusals%u/ALL12memory+16execution-result fields/8partial-map failures+retention/8native lifetime losses/4closed-END misbindings;ZERO following physical IO/reset/retry/restore\n",refused);
#ifdef PIONEER_Q38_MIXED_LIVE_END_FIXTURE
 assert(argc==8 && (!strcmp(argv[7],"--clock-end") || !strcmp(argv[7],"--final-end")) && !execute && !faults);
#ifndef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
 assert(CloseBulkScratch(c,true) && !c.bulk.ready && !c.bulk.dirty && !c.bulk.unsafe);
 assert(PioneerQ38DispatchPeerLeaseEnd(io,c.baseline.baseline.baseline.noc,c.lease) && !c.lease.ready && c.lease.niu_restored && c.lease.tlb_restored && c.lease.l1_restored);
 for(unsigned fault=0;fault<5;fault++)NativeFactoryFork([&]{
  c.q38Clock.Fault(fault);auto physical=transport.Touches();auto before=c.q38Clock.IO();
  assert(!FinishQ38NormalClock(c,true) && c.q38Clock.Unsafe());
  auto after=c.q38Clock.IO();assert(after>=before && !FinishQ38NormalClock(c,true) && c.q38Clock.IO()==after && transport.Touches()==physical);
 });
 assert(FinishQ38NormalClock(c,true) && !c.q38Clock.RequiresExplicitEnd() && !c.q38Clock.Unsafe()
  && c.q38Clock.Snapshot().state==TT_ARC_CLOCK_RESTORED && c.q38Clock.Snapshot().restored
  && c.q38Clock.Snapshot().after_mhz==800 && c.q38Clock.Notifications()==5 && c.q38Clock.Unmaps()==1);
#endif
#ifdef PIONEER_Q38_MIXED_LIVE_FINAL_RELEASE
 io.InitFinalVmViews();c.transaction.map_attempted=c.transaction.config_attempted=true;c.pci.version=1;c.pci.state=TT_PIONEER_MMIO_READ_OK;
 for(unsigned i=0;i<6;i++)c.pci.bars_before[i]=i%2?0:0x0c;c.pci.bridge_before[0]=0x1fff1;
 for(unsigned bad=0;bad<8;bad++)NativeFactoryFork([&]{
  if(bad==0)currentTeam++;if(bad==1)sRuntimeOwner=nullptr;if(bad==2)io.q38MutableContext=nullptr;if(bad==3)c.hashes=nullptr;
  if(bad==4)c.q38EmbeddingModel->token++;if(bad==5)c.bankStorageOnly=false;if(bad==6)io.runtimeCore=119;if(bad==7)sReloadGeneration++;
  auto clockIO=c.q38Clock.IO(),physical=transport.Touches();tt_resident_session_result closed={};NativeFinalTail(&f.cookie,closed,false);
  assert(f.cookie.context==&c && c.dispatchOwnerRetained && sColdRecoveryRequired && !finalDestroyed && !finalUnreserved && finalRemoved.empty() && !io.finalPciWrites);
  NativeFinalTail(&f.cookie,closed,false);assert(clockIO==c.q38Clock.IO() && physical==transport.Touches() && finalRemoved.empty() && !io.finalPciWrites);
 });
 for(unsigned fault=0;fault<5;fault++)NativeFactoryFork([&]{
  c.q38Clock.Fault(fault);tt_resident_session_result closed={};NativeFinalTail(&f.cookie,closed,false);
  assert(f.cookie.context==&c && sRuntimeOwner==&f.cookie && c.dispatchOwnerRetained && sColdRecoveryRequired && !finalDestroyed && !finalUnreserved && finalRemoved.empty() && !io.finalPciWrites);
  auto clockIO=c.q38Clock.IO();NativeFinalTail(&f.cookie,closed,false);assert(c.q38Clock.IO()==clockIO && finalRemoved.empty() && !io.finalPciWrites);
 });
 for(unsigned fail=1;fail<=33;fail++)NativeFactoryFork([&]{
  finalDeleteFailure=fail;tt_resident_session_result closed={};
  NativeFinalTail(&f.cookie,closed,false);
  assert(f.cookie.context==&c && sRuntimeOwner==&f.cookie && !finalDestroyed && !finalUnreserved && c.dispatchOwnerRetained && sColdRecoveryRequired && finalDeleteFailed && finalRemoved.size()==fail);
  auto clockIO=c.q38Clock.IO();auto before=finalRemoved;auto writes=io.finalPciWrites;NativeFinalTail(&f.cookie,closed,false);
  assert(before==finalRemoved && writes==io.finalPciWrites && c.q38Clock.IO()==clockIO);
 });
 for(unsigned fail=1;fail<=11;fail++)NativeFactoryFork([&]{
  finalPciFailure=fail;tt_resident_session_result closed={};
  NativeFinalTail(&f.cookie,closed,false);
  assert(f.cookie.context==&c && sRuntimeOwner==&f.cookie && !finalDestroyed && !finalUnreserved && c.dispatchOwnerRetained && sColdRecoveryRequired && finalPciFailed && finalRemoved.size()==30 && io.finalPciWrites==fail);
  auto clockIO=c.q38Clock.IO();auto reads=io.finalPciReads;NativeFinalTail(&f.cookie,closed,false);assert(reads==io.finalPciReads && finalRemoved.size()==30 && c.q38Clock.IO()==clockIO);
 });
 assert(f.c.release()==&c);tt_resident_session_result closed={};NativeFinalTail(&f.cookie,closed,false);
 assert(!f.cookie.context && !sRuntimeOwner && finalDestroyed==1 && finalUnreserved==1 && io.q38MixedLiveFinalUnmapped && finalRemoved.size()==33 && io.finalPciWrites==11 && closed.state==TT_RESIDENT_SESSION_CLOSED && closed.host_released && closed.pci_restored && !closed.cold);
 puts("PASS GENUINE native closed -> VERBATIM actual owned END prefix+footer/bulk-root closure/real private clock/final scope/30VM Unmap/11PCI writes+proof/actual3host release methods/unreserve+cookie-global clear+mixed-prefix destruction;8binding+5clock+33VM-host+11PCI refusals retain context/reservation and fence later disposal/retry;SIMULATED OS/PCI,NO fresh diagnostic recheck/complete callseal/physical qualification");
#else
 // The ACTUAL PCI transaction creates its own pessimistic status marker
 // across the REAL closed-native+ended-clock boundary. No private setter.
 c.q38EndPhase=50;c.transaction.map_attempted=c.transaction.config_attempted=true;c.pci.version=1;c.pci.state=TT_PIONEER_MMIO_READ_OK;
 auto clockIO=c.q38Clock.IO(),physical=transport.Touches();
 NativeClockPciFailureEndpoint endpoint{c};
 assert(!PioneerQ38DispatchMmioEnd(endpoint,c.pci,c.transaction) && endpoint.attempts==1 && !c.transaction.config_attempted
  && PioneerQ38MixedLiveFinalPciUncertain(c) && c.Unsafe() && c.pci.state==TT_PIONEER_MMIO_RESTORE_FAILED && !c.pci.restored);
 assert(!PioneerQ38DispatchMmioEnd(endpoint,c.pci,c.transaction) && endpoint.attempts==1);
 assert(c.q38Clock.IO()==clockIO && transport.Touches()==physical); // necessary predicate itself performs ZERO clock/card IO
 puts("PASS ACTUAL native held-state/Dispatch END/bulk+root lease CLOSE -> ACTUAL FinishNormalClock/private clock lease IDLE+800MHz/5messages+1virtualUnmap;5 raw ARC/VM/unknown failures fence further clock+compute IO;SIMULATED OS/MMIO/mailbox,NO ready setter/full final PCI-host-context release or physical proof");
 puts("PASS actual DispatchMmioEnd marker+necessary predicate with GENUINE private closed native+ended clock:pre-Unmap marker admitted/simulated first VM failure remains unsafe/PCI status NEVER cleared/ZERO later PCI,card,clock or retry IO;NOT complete final Unmap/host-release authority");
#endif
#endif
 io.mixedLiveTransport=nullptr;
}
