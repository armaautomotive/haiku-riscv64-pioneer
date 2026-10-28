/* SPDX-License-Identifier: MIT */
// Actual new native factories/scopes/private producers/journal/dispatch END.
// ONLY OS/PCI/L1/raw learned bytes/time endpoints are simulated. No ready,
// native phase,completed producer,processor PC or nonce setter exists.
#define PIONEER_Q38_RECURRENT_PREFIX_NATIVE_FACTORY_FIXTURE
#define PIONEER_Q38_RECURRENT_PREFIX_INITIALIZATION_FIXTURE
#define PIONEER_Q38_RECURRENT_BINDING_FIXTURE_ONLY
#include "pioneer_q38_recurrent_prefix_native_binding_test.cpp"
#include <sys/wait.h>
#include <unistd.h>
#include "pioneer_q38_recurrent_prefix_native_final_fixture.inc"
bool PioneerSessionIO::UseRecurrentPrefixIO()const{
 auto* c=q38MutableContext;auto* state=c?c->q38RecurrentPrefixNative:nullptr;
 return recurrentTransport && state && state->phase>=PioneerQ38RecurrentPrefixNativeState::DispatchPreparing;
} // TEST raw endpoint choice observes the actual private phase,NEVER sets it
void PioneerSessionIO::ConfigureRecurrentPrefixTransport(){
 auto* c=q38MutableContext;auto* state=c?c->q38RecurrentPrefixNative:nullptr;
 assert(c && state && state->phase==PioneerQ38RecurrentPrefixNativeState::MemoryInstalled && state->plan && recurrentTransport
  && state->InitializationsClosed(c->banks,c->baseline.baseline.baseline.baseline.dram));
 struct Bytes:PrefixInitializedByteSource {
  const PioneerSessionIO& io;explicit Bytes(const PioneerSessionIO& p):io(p){}
  bool Recurrent(unsigned bank,uint32_t address,uint32_t* output,unsigned words)const override{
   if(!output || !words || (address&3))return false;
   for(unsigned i=0;i<672;i++){const auto& e=io.recurrentPrefixGdnExtents[i];
    if(e.bank!=bank || !e.bytes || address<e.address || address-e.address>=e.bytes || words>(e.bytes-(address-e.address))/4)continue;
    unsigned first=io.recurrentPrefixGdnFirst[i]*1024+(address-e.address)/4;
    if(first>io.recurrentPrefixGdnWords.size() || words>io.recurrentPrefixGdnWords.size()-first)return false;
    for(unsigned w=0;w<words;w++)output[w]=io.recurrentPrefixGdnWords[first+w];return true;
   }return false;
  }
  bool History(unsigned bank,uint32_t address,uint32_t* output,unsigned words)const override{
   if(!output || !words || (address&3))return false;
   for(unsigned i=0;i<336;i++){const auto& e=io.recurrentPrefixHistoryExtents[i];
    if(e.bank!=bank || !e.bytes || address<e.address || address-e.address>=e.bytes || words>(e.bytes-(address-e.address))/4)continue;
    unsigned first=i*5120+(address-e.address)/4;
    if(first>io.recurrentPrefixHistoryWords.size() || words>io.recurrentPrefixHistoryWords.size()-first)return false;
    for(unsigned w=0;w<words;w++)output[w]=io.recurrentPrefixHistoryWords[first+w];return true;
   }return false;
  }
 }bytes(*this);
 assert(recurrentTransport->Configure(*state->plan,wholeWorkspace.data(),5120,bytes));
} // raw actual zero/QPE1 output bytes only,not a native producer grant
template<class Check> static void RecurrentNativeFork(Check check){
 fflush(nullptr);pid_t child=fork();assert(child>=0);if(!child){check();_exit(0);}
 int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
}
int main(int argc,char** argv){
 assert(argc==5 || (argc==6 && (!strcmp(argv[5],"--execute") || !strcmp(argv[5],"--faults") || !strcmp(argv[5],"--fresh-held-end") || !strcmp(argv[5],"--fresh-execute-end"))));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 const bool execute=argc==6 && (!strcmp(argv[5],"--execute") || !strcmp(argv[5],"--fresh-execute-end")),faults=argc==6 && !strcmp(argv[5],"--faults");
 RecurrentBindingFixture f(argv[1],argv[2]);auto& c=*f.c;auto& io=f.io;
#ifdef PIONEER_Q38_RECURRENT_PREFIX_FRESH_FINAL_TEST
 (void)&AcquireQ38DeltaLock;
 auto computeScope=std::make_unique<PioneerQ38GdnCallScope>(&f.cookie);
 assert(computeScope->Status()==B_OK && computeScope->Context()==&c && finalLockHeld && c.q38GdnCallThread==currentThread);
#endif
 f.Prefix();
 assert(BindQ38RecurrentPrefixNative(c));auto* state=c.q38RecurrentPrefixNative;
 PioneerQ38StateZeroResult gdn;PioneerQ38ConvHistoryZeroResult history;
 assert(InitializeQ38RecurrentPrefixState(c,gdn) && InitializeQ38RecurrentPrefixHistory(c,history)
  && state->InitializationsClosed(c.banks,c.baseline.baseline.baseline.baseline.dram));
 puts("PASS QPX genuine native SAME completed QPE1 + actual672GDN/336history zero/private clean publication;NO transferred ready facade");
 RecurrentPrefixNativeTransport transport(argv[3],argv[4]);
 std::vector<uint32> addresses,values;auto seed=[&](uint32 a,uint32 v){addresses.push_back(a);values.push_back(v);};
 seed(0x6000,io.entry);for(unsigned i=0;i<io.prefix.size();i++)seed(0x64000+i*4,io.prefix[i]);
 for(unsigned i=0;i<io.view.size();i++)seed(TT_Q38_PANEL_VIEW_BASE+i*4,io.view[i]);
 for(unsigned i=0;i<2048;i++)seed(0x17e000+i*4,io.destination[i]);
 uint32 source[16],command[10],counts[8];
 for(unsigned i=0;i<16;i++)source[i]=i?io.source[i]:io.config;
 for(unsigned i=0;i<10;i++)command[i]=io.command[i];
 for(unsigned i=0;i<8;i++)counts[i]=io.PanelIO::ResidentNocRead(PioneerQ38PanelCounter(i));
 transport.SeedRoot(addresses.data(),values.data(),unsigned(addresses.size()),source,command,counts);
 const auto before=transport.Fingerprint();auto learned=f.learned;io.recurrentTransport=&transport;
 PioneerQ38RecurrentPrefixMemoryResult memory;
 if(faults)for(unsigned null=0;null<2;null++)for(unsigned map=0;map<4;map++)RecurrentNativeFork([&]{
  if(null)io.recurrentNullMap=int(map);else io.recurrentFailMap=int(map);
  assert(!PrepareQ38RecurrentPrefixNativeHeld(c,memory) && io.recurrentMapFaultFence && io.touches==io.recurrentMapFaultFence
   && state->Unsafe() && c.dispatchOwnerRetained && c.banks.Quarantined() && c.q38Clock.quarantined
   && f.transfer.quarantined && f.wide.quarantined && sColdRecoveryRequired && !transport.Resets());
  auto physical=transport.Touches(),touches=io.touches;
  assert(!PrepareQ38RecurrentPrefixNativeHeld(c,memory) && !CloseQ38RecurrentPrefixNativeHeld(c,memory) && !EndFactory(c)
   && touches==io.touches && physical==transport.Touches());
  printf("PASS QPX actual native partial map fault%u/null%u/ALL owners retained/ZERO later IO or retry\n",map,null);
 });
 bool prepared=PrepareQ38RecurrentPrefixNativeHeld(c,memory);
 if(!prepared)fprintf(stderr,"QPX_NATIVE_HELD_FAILED prepared%u phase%u worker%u word%u unsafe%u\n",
  c.q38Dispatch?c.q38Dispatch->prepared:0,memory.phase,memory.worker,memory.word,state->Unsafe());
 assert(prepared && c.q38Dispatch && c.q38Dispatch->prepared==120 && c.q38DispatchRoutes && !state->Unsafe()
  && memory.savedWords==TT_Q38_RECURRENT_PREFIX_SAVED_WORDS && memory.verifiedOriginalWords==TT_Q38_RECURRENT_PREFIX_SAVED_WORDS
  && memory.installedWords==TT_Q38_RECURRENT_PREFIX_CODE_WORDS && io.mapCalls==5 && !transport.Resets());
 puts("PASS QPX actual native ALL120 probes/peer leases/held roles/4new mappings/complete prefix journal+program install;NO reset or synthetic permission");
 PioneerQ38RecurrentPrefixResult execution;
 if(execute){
  io.ConfigureRecurrentPrefixTransport();transport.Observe(execution);
  bool ready=PrepareQ38RecurrentPrefixNativeExecution(c,execution);
  if(!ready){fprintf(stderr,"QPX_NATIVE_PREPARE_FAILED phase%u worker%u word%u\n",execution.phase,execution.worker,execution.word);transport.Diagnostic();}
  assert(ready && !state->Unsafe() && !state->Closed());
  bool ran=RunQ38RecurrentPrefixNativeExecution(c,execution);
  if(!ran){fprintf(stderr,"QPX_NATIVE_RUN_FAILED phase%u worker%u word%u\n",execution.phase,execution.worker,execution.word);transport.Diagnostic();}
  assert(ran);transport.VerifyRun();
  for(unsigned bad=0;bad<6;bad++)RecurrentNativeFork([&]{
   if(bad==0)io.q38MutableContext=nullptr;if(bad==1)sRuntimeOwner=nullptr;if(bad==2)c.hashes=nullptr;
   if(bad==3)currentTeam++;if(bad==4)f.transfer.ready=false;if(bad==5)c.span.closed=false;
   auto physical=transport.Touches(),touches=io.touches,resets=transport.Resets();
   assert(!CloseQ38RecurrentPrefixNativeExecution(c,execution,memory) && state->Unsafe() && c.dispatchOwnerRetained
    && c.banks.Quarantined() && c.q38Clock.quarantined && f.transfer.quarantined && f.wide.quarantined && sColdRecoveryRequired);
   assert(!CloseQ38RecurrentPrefixNativeExecution(c,execution,memory) && !EndFactory(c)
    && physical==transport.Touches() && touches==io.touches && resets==transport.Resets());
   printf("PASS QPX actual post3layer native lifetime fault%u/FD+model+clock+ALLhosts retained/ZERO later physical IO\n",bad);
  });
  bool closed=CloseQ38RecurrentPrefixNativeExecution(c,execution,memory);
  if(!closed){fprintf(stderr,"QPX_NATIVE_CLOSE_FAILED phase%u worker%u word%u holds%u stopped%u\n",
   execution.phase,execution.worker,execution.word,execution.held,execution.stopped);transport.Diagnostic();}
  assert(closed && execution.completed==119 && execution.stopped==119 && execution.held==120 && execution.packets==57);
  transport.VerifyClosed();
 }else assert(CloseQ38RecurrentPrefixNativeHeld(c,memory) && !transport.Resets());
 assert(state->Closed() && !state->Unsafe() && memory.restoredWords==TT_Q38_RECURRENT_PREFIX_SAVED_WORDS
  && memory.verifiedOriginalWords==2*TT_Q38_RECURRENT_PREFIX_SAVED_WORDS);
 bool ended=EndFactory(c);
 if(!ended)fprintf(stderr,"QPX_NATIVE_DISPATCH_END_FAILED phase%u worker%u detail%08x/%08x/%08x\n",
  c.q38Dispatch->closePhase,c.q38Dispatch->closeWorker,c.q38Dispatch->closeDetail[0],c.q38Dispatch->closeDetail[1],c.q38Dispatch->closeDetail[2]);
 assert(ended && c.q38Dispatch->closed && state->Closed() && !c.dispatchOwnerRetained && !sColdRecoveryRequired
  && transport.Fingerprint()==before && f.learned==learned && !io.pciWrites && !io.HostReadMock::bmeWrites);
 printf("PASS DISTINCT QPX actual native %s/SAME QPE1+672+336init/real scopes+leases+maps+3layerplan/full restore+secondproof/dispatch END;SIMULATED OS+raw learned/MMIO,NOT final clock/VM/PCI/host disposal,original-model,full64,physical,Work or TPS\n",
  execute?"autonomous3RUN+119STOP+ALL120holds":"HELD no launch");
#ifdef PIONEER_Q38_RECURRENT_PREFIX_FINAL_RELEASE
#include "pioneer_q38_recurrent_prefix_native_final_checks.inc"
#endif
 io.recurrentTransport=nullptr;
}
