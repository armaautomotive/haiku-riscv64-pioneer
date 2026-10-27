/* SPDX-License-Identifier: MIT */
// Genuine native factories/scopes/held journal/complete Dispatch END.
// Only OS/PCI/L1/time endpoints are simulated; NO native phase/ready setters.
#include <cstdint>
#include "../PioneerQ38MixedLiveBlockRootLayout.h"
#define PIONEER_Q38_EMBEDDING_FACTORY_FIXTURE_ONLY
#include "pioneer_q38_embedding_native_factory_test.cpp"
#include <sys/wait.h>
#include <unistd.h>
template<class Check> static void NativeFactoryFork(Check check){
 fflush(nullptr);pid_t pid=fork();assert(pid>=0);if(!pid){check();_exit(0);}
 int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
}
int main(int argc,char** argv){
 assert(argc==7 || (argc==8 && (!strcmp(argv[7],"--execute") || !strcmp(argv[7],"--faults") || !strcmp(argv[7],"--validation-readback"))));setvbuf(stdout,nullptr,_IOLBF,0);
 bool readback=argc==8 && !strcmp(argv[7],"--validation-readback");
 bool execute=readback || (argc==8 && !strcmp(argv[7],"--execute")),faults=argc==8 && !strcmp(argv[7],"--faults");unsigned refused=0;
#ifndef PIONEER_Q38_MIXED_LIVE_VALIDATION_READBACK
 assert(!readback);
#endif
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;
 (void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 NativeFactoryFixture f(argv[1],argv[2]);auto& c=*f.c;auto& io=f.io;
 auto loseScope=[&](unsigned bad){
  if(bad==0)io.q38MutableContext=nullptr;if(bad==1)sRuntimeOwner=nullptr;if(bad==2)c.q38Clock.value.busy_mhz=0;
  if(bad==3)c.q38EmbeddingModel->token++;if(bad==4)c.hashes=nullptr;if(bad==5)currentTeam++;
  if(bad==6)f.transfer.ready=false;if(bad==7)c.span.closed=false;
 };
 c.q38Clock.value.busy_mhz=1400;auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
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
 io.mixedLiveTransport=nullptr;
}
