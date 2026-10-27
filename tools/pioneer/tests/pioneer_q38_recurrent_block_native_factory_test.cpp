/* SPDX-License-Identifier: MIT */
// Actual new native factories/scopes/private producers/journal/dispatch END.
// ONLY OS/PCI/L1/raw learned bytes/time endpoints are simulated. No ready,
// native phase,completed producer,processor PC or nonce setter exists.
#define PIONEER_Q38_RECURRENT_BLOCK_NATIVE_FACTORY_FIXTURE
#define PIONEER_Q38_RECURRENT_BLOCK_INITIALIZATION_FIXTURE
#define PIONEER_Q38_RECURRENT_BINDING_FIXTURE_ONLY
#include "pioneer_q38_recurrent_block_native_binding_test.cpp"
#include <sys/wait.h>
#include <unistd.h>
#include "pioneer_q38_recurrent_block_native_final_fixture.inc"
#ifdef PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST
#include "pioneer_q38_recurrent_block_original_native.h"
#endif
bool PioneerSessionIO::UseRecurrentBlockIO()const{
 auto* c=q38MutableContext;auto* state=c?c->q38RecurrentBlockNative:nullptr;
 return recurrentTransport && state && state->phase>=PioneerQ38RecurrentBlockNativeState::DispatchPreparing;
} // TEST raw endpoint choice observes the actual private phase,NEVER sets it
void PioneerSessionIO::ConfigureRecurrentBlockTransport(){
 auto* c=q38MutableContext;auto* state=c?c->q38RecurrentBlockNative:nullptr;
 assert(c && state && state->phase==PioneerQ38RecurrentBlockNativeState::MemoryInstalled && state->plan && recurrentTransport
  && state->InitializationsClosed(c->banks,c->baseline.baseline.baseline.baseline.dram));
 struct Bytes:RecurrentInitializedByteSource {
  const PioneerSessionIO& io;explicit Bytes(const PioneerSessionIO& p):io(p){}
  bool Recurrent(unsigned bank,uint32_t address,uint32_t* output,unsigned words)const override{
   if(!output || !words || (address&3))return false;
   for(unsigned i=0;i<672;i++){const auto& e=io.recurrentBlockGdnExtents[i];
    if(e.bank!=bank || !e.bytes || address<e.address || address-e.address>=e.bytes || words>(e.bytes-(address-e.address))/4)continue;
    unsigned first=io.recurrentBlockGdnFirst[i]*1024+(address-e.address)/4;
    if(first>io.recurrentBlockGdnWords.size() || words>io.recurrentBlockGdnWords.size()-first)return false;
    for(unsigned w=0;w<words;w++)output[w]=io.recurrentBlockGdnWords[first+w];return true;
   }return false;
  }
  bool History(unsigned bank,uint32_t address,uint32_t* output,unsigned words)const override{
   if(!output || !words || (address&3))return false;
   for(unsigned i=0;i<336;i++){const auto& e=io.recurrentBlockHistoryExtents[i];
    if(e.bank!=bank || !e.bytes || address<e.address || address-e.address>=e.bytes || words>(e.bytes-(address-e.address))/4)continue;
    unsigned first=i*5120+(address-e.address)/4;
    if(first>io.recurrentBlockHistoryWords.size() || words>io.recurrentBlockHistoryWords.size()-first)return false;
    for(unsigned w=0;w<words;w++)output[w]=io.recurrentBlockHistoryWords[first+w];return true;
   }return false;
  }
 }bytes(*this);
 assert(recurrentTransport->Configure(*state->plan,wholeWorkspace.data(),5120,bytes));
} // raw actual zero/QPE1 output bytes only,not a native producer grant
template<class Check> static void RecurrentNativeFork(Check check){
 fflush(nullptr);pid_t child=fork();assert(child>=0);if(!child){check();_exit(0);}
 int status=0;assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
}
#ifndef PIONEER_Q38_RECURRENT_BLOCK_CONTROL_FIXTURE_ONLY
#ifdef PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST
int ExecuteRecurrentOriginalNativeScenario(const char* const* images,RecurrentBlockRawLearnedBytes& original,uint32_t* output,unsigned outputWords,RecurrentOriginalStageSink* stageSink){
 if(!images || !output || outputWords!=5120 || (uintptr_t(output)&3) || uintptr_t(output)>UINTPTR_MAX-20480 || !original.Ready())return 1;
 for(unsigned i=0;i<4;i++)if(!images[i] || !*images[i])return 1;
 const int argc=6;const char* argv[]={"original-native",images[0],images[1],images[2],images[3],"--fresh-execute-end"};
 auto sourcePlan=std::make_unique<PioneerQ38CanonicalModelPlan>();if(!sourcePlan->Build())return 1;
 const auto* tile=sourcePlan->ProjectionStripe(64,0,0);std::vector<uint8_t> embeddingBytes(46080);
 if(!tile || tile->firstRow)return 1;
 for(unsigned at=0;at<embeddingBytes.size();at+=16384){
  unsigned count=unsigned(embeddingBytes.size())-at;if(count>16384)count=16384;
  if(!original.Ready() || !original.Read(PioneerQ38ProjectionRoute(tile->bank),sourcePlan->Base+tile->offset+at,embeddingBytes.data()+at,count))return 1;
 }
 if(!original.Ready())return 1;
 std::array<uint32_t,5120> originalCapture; // caller output untouched until complete positive END
#else
int main(int argc,char** argv){
#endif
 assert(argc==5 || (argc==6 && (!strcmp(argv[5],"--execute") || !strcmp(argv[5],"--faults") || !strcmp(argv[5],"--fresh-held-end") || !strcmp(argv[5],"--fresh-execute-end"))));setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;
 bool execute=argc==6 && (!strcmp(argv[5],"--execute") || !strcmp(argv[5],"--fresh-execute-end")),faults=argc==6 && !strcmp(argv[5],"--faults");
 RecurrentBindingFixture f(argv[1],argv[2]);auto& c=*f.c;auto& io=f.io;
#ifdef PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST
 io.originalEmbeddingWeights=embeddingBytes.data(); // original PACKED BYTES,never embedding products
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_FRESH_FINAL_TEST
 assert(argc==6 && (!strcmp(argv[5],"--fresh-held-end") || !strcmp(argv[5],"--fresh-execute-end")));
 (void)&AcquireQ38DeltaLock;
 auto computeScope=std::make_unique<PioneerQ38GdnCallScope>(&f.cookie);
 assert(computeScope->Status()==B_OK && computeScope->Context()==&c && finalLockHeld && c.q38GdnCallThread==currentThread);
#else
 assert(argc!=6 || (strcmp(argv[5],"--fresh-held-end") && strcmp(argv[5],"--fresh-execute-end")));
#endif
 f.Prefix();
 assert(BindQ38RecurrentBlockNative(c));auto* state=c.q38RecurrentBlockNative;
 PioneerQ38StateZeroResult gdn;PioneerQ38ConvHistoryZeroResult history;
 assert(InitializeQ38RecurrentBlockState(c,gdn) && InitializeQ38RecurrentBlockHistory(c,history)
  && state->InitializationsClosed(c.banks,c.baseline.baseline.baseline.baseline.dram));
 puts("PASS QRB genuine native SAME completed QPE1 producer + actual672GDN/336history zero instruction leases/private clean publication;NO transferred ready facade");
 RecurrentBlockNativeTransport transport(argv[3],argv[4]);
#ifdef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK
 assert(execute);transport.EnableValidationReadback();
 std::vector<uint32_t> validationCapture(5120,0xdeadbeef);PioneerQ38RecurrentBlockReadbackResult validation;
#endif
#ifdef PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST
 assert(original.Ready() && transport.AttachOriginalLearned(original));
 puts("STAGED ORIGINAL token0 packed QPE1 tile+ALL14 recurrent layer0 tensors/raw57spans;synthetic learned fallback DISABLED,NO expected products/native ready grant");
#endif
 std::vector<uint32> addresses,values;auto seed=[&](uint32 a,uint32 v){addresses.push_back(a);values.push_back(v);};
 seed(0x6000,io.entry);for(unsigned i=0;i<io.prefix.size();i++)seed(0x64000+i*4,io.prefix[i]);
 for(unsigned i=0;i<io.view.size();i++)seed(TT_Q38_PANEL_VIEW_BASE+i*4,io.view[i]);
 for(unsigned i=0;i<2048;i++)seed(0x17e000+i*4,io.destination[i]);
 uint32 source[16],command[10],counts[8];
 for(unsigned i=0;i<16;i++)source[i]=i?io.source[i]:io.config;
 for(unsigned i=0;i<10;i++)command[i]=io.command[i];
 for(unsigned i=0;i<8;i++)counts[i]=io.PanelIO::ResidentNocRead(PioneerQ38PanelCounter(i));
 transport.SeedRoot(addresses.data(),values.data(),unsigned(addresses.size()),source,command,counts);
 auto before=transport.Fingerprint();auto learned=f.learned;io.recurrentTransport=&transport;
 PioneerQ38RecurrentBlockMemoryResult memory;
 if(faults)for(unsigned null=0;null<2;null++)for(unsigned map=0;map<4;map++)RecurrentNativeFork([&]{
  if(null)io.recurrentNullMap=int(map);else io.recurrentFailMap=int(map);
  assert(!PrepareQ38RecurrentBlockNativeHeld(c,memory) && io.recurrentMapFaultFence && io.touches==io.recurrentMapFaultFence
   && state->Unsafe() && c.dispatchOwnerRetained && c.banks.Quarantined() && c.q38Clock.quarantined
   && f.transfer.quarantined && f.wide.quarantined && sColdRecoveryRequired && !transport.Resets());
  auto physical=transport.Touches(),touches=io.touches;
  assert(!PrepareQ38RecurrentBlockNativeHeld(c,memory) && !CloseQ38RecurrentBlockNativeHeld(c,memory) && !EndFactory(c)
   && touches==io.touches && physical==transport.Touches());
 });
 bool prepared=PrepareQ38RecurrentBlockNativeHeld(c,memory);
 if(!prepared)fprintf(stderr,"QRB_NATIVE_HELD_FAILED prepared%u phase%u worker%u word%u unsafe%u\n",c.q38Dispatch?c.q38Dispatch->prepared:0,memory.phase,memory.worker,memory.word,state->Unsafe());
 assert(prepared && c.q38Dispatch && c.q38Dispatch->prepared==120 && c.q38DispatchRoutes && !state->Unsafe()
  && memory.savedWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS && memory.verifiedOriginalWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS
  && memory.installedWords==TT_Q38_RECURRENT_BLOCK_CODE_WORDS && io.mapCalls==5 && !transport.Resets());
 puts("PASS QRB actual native ALL120 probes/peer leases/held role prep/4new mappings/full journal+11signed code sections;NO reset or synthetic permission");
 PioneerQ38RecurrentBlockResult execution;
 if(execute){
  io.ConfigureRecurrentBlockTransport();transport.Observe(execution);
  bool ready=PrepareQ38RecurrentBlockNativeExecution(c,execution);
  if(!ready){fprintf(stderr,"QRB_NATIVE_PREPARE_FAILED phase%u worker%u word%u\n",execution.phase,execution.worker,execution.word);transport.Diagnostic();}
  assert(ready && !state->Unsafe() && !state->Closed());
  bool ran=RunQ38RecurrentBlockNativeExecution(c,execution);
  if(!ran){fprintf(stderr,"QRB_NATIVE_RUN_FAILED phase%u worker%u word%u\n",execution.phase,execution.worker,execution.word);transport.Diagnostic();}
  assert(ran);transport.VerifyRun();
#ifdef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK
  auto inert=[&](uint32_t* values,unsigned words){auto physical=transport.Touches(),touches=io.touches;
   assert(!CloseQ38RecurrentBlockNativeValidation(c,execution,memory,values,words,validation)
    && !state->Closed() && !state->Unsafe() && !validation.started
    && physical==transport.Touches() && touches==io.touches && !transport.ValidationReadbackPackets());
  };
  inert(nullptr,5120);inert(validationCapture.data(),5119);inert(reinterpret_cast<uint32_t*>(&c),5120);
  inert(reinterpret_cast<uint32_t*>(state),5120);inert(reinterpret_cast<uint32_t*>(c.q38EmbeddingModel),5120);
  inert(reinterpret_cast<uint32_t*>(&execution),5120);inert(reinterpret_cast<uint32_t*>(&memory),5120);inert(reinterpret_cast<uint32_t*>(&validation),5120);
  puts("PASS actual native validation request8 inert CPU refusals AFTER genuine whole RUN;NO physical IO/phase change/retention or output publication");
#endif
  for(unsigned bad=0;bad<6;bad++)RecurrentNativeFork([&]{
   if(bad==0)io.q38MutableContext=nullptr;if(bad==1)sRuntimeOwner=nullptr;if(bad==2)c.hashes=nullptr;
   if(bad==3)currentTeam++;if(bad==4)f.transfer.ready=false;if(bad==5)c.span.closed=false;
   auto physical=transport.Touches(),touches=io.touches,resets=transport.Resets();
#ifdef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK
   auto close=[&]{return CloseQ38RecurrentBlockNativeValidation(c,execution,memory,validationCapture.data(),unsigned(validationCapture.size()),validation);};
#else
   auto close=[&]{return CloseQ38RecurrentBlockNativeExecution(c,execution,memory);};
#endif
   assert(!close() && state->Unsafe() && c.dispatchOwnerRetained
    && c.banks.Quarantined() && c.q38Clock.quarantined && f.transfer.quarantined && f.wide.quarantined && sColdRecoveryRequired);
   assert(!close() && !EndFactory(c)
    && physical==transport.Touches() && touches==io.touches && resets==transport.Resets());
#ifdef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK
   assert(!validation.started && !validation.completed && !transport.ValidationReadbackPackets());
   for(auto value:validationCapture)assert(value==0xdeadbeef);
#endif
  });
#ifdef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK
  for(unsigned fault=1;fault<=3;fault++)RecurrentNativeFork([&]{
   transport.FailValidationReadback(fault);
   auto close=[&]{return CloseQ38RecurrentBlockNativeValidation(c,execution,memory,validationCapture.data(),unsigned(validationCapture.size()),validation);};
   assert(!close() && transport.ValidationReadbackFaultObserved() && transport.ValidationReadbackPackets()==1
    && state->Unsafe() && c.dispatchOwnerRetained && c.banks.Quarantined() && c.q38Clock.quarantined
    && f.transfer.quarantined && f.wide.quarantined && sColdRecoveryRequired && validation.started && !validation.completed
    && execution.stopped==119 && execution.held==120 && !memory.restoredWords);
   for(auto value:validationCapture)assert(value==0xdeadbeef);
   auto physical=transport.Touches(),touches=io.touches,resets=transport.Resets();
   assert(!close() && !CloseQ38RecurrentBlockNativeExecution(c,execution,memory) && !EndFactory(c)
    && physical==transport.Touches() && touches==io.touches && resets==transport.Resets());
   printf("PASS actual native readback fault%u AFTER real119STOP+120holds/FD+clock+hosts retained/ZERO following physical IO/restore/retry or caller publication\n",fault);
  });
  bool closed=CloseQ38RecurrentBlockNativeValidation(c,execution,memory,validationCapture.data(),unsigned(validationCapture.size()),validation);
#else
  bool closed=CloseQ38RecurrentBlockNativeExecution(c,execution,memory);
#endif
  if(!closed){fprintf(stderr,"QRB_NATIVE_CLOSE_FAILED phase%u worker%u word%u holds%u stopped%u\n",execution.phase,execution.worker,execution.word,execution.held,execution.stopped);transport.Diagnostic();}
  assert(closed && execution.completed==119 && execution.stopped==119 && execution.held==120);transport.VerifyClosed();
#ifdef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK
  assert(validation.completed && validation.pages==5 && validation.words==5120);transport.VerifyValidationReadback(validationCapture.data(),unsigned(validationCapture.size()));
  puts("PASS SAME actual native owner/FD+team+thread+clock+leases/private scoped5READ/5120 actual GDDR output bits AFTER realSTOP+120holds/full11406904 restoration+secondproof;RAW endpoints SIMULATED,NOT physical card/Work/TPS");
#endif
 }else assert(CloseQ38RecurrentBlockNativeHeld(c,memory) && !transport.Resets());
 assert(state->Closed() && !state->Unsafe() && memory.restoredWords==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS
  && memory.verifiedOriginalWords==2*TT_Q38_RECURRENT_BLOCK_SAVED_WORDS);
#ifdef PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST
#ifdef PIONEER_Q38_RECURRENT_BLOCK_VALIDATION_READBACK
 assert(original.Ready() && validation.completed);std::memcpy(originalCapture.data(),validationCapture.data(),sizeof(originalCapture));
#else
 assert(original.Ready() && transport.CaptureOriginalOutput(originalCapture.data(),unsigned(originalCapture.size())));
#endif
#endif
 bool ended=EndFactory(c);
 if(!ended)fprintf(stderr,"QRB_NATIVE_DISPATCH_END_FAILED phase%u worker%u detail%08x/%08x/%08x\n",c.q38Dispatch->closePhase,c.q38Dispatch->closeWorker,c.q38Dispatch->closeDetail[0],c.q38Dispatch->closeDetail[1],c.q38Dispatch->closeDetail[2]);
 assert(ended && c.q38Dispatch->closed && state->Closed() && !c.dispatchOwnerRetained && !sColdRecoveryRequired
  && transport.Fingerprint()==before && f.learned==learned && !io.pciWrites && !io.HostReadMock::bmeWrites);
 printf("PASS DISTINCT QRB genuine native %s/SAME embedding+672+336init producers/actual scopes+leases+map/journal/full restore+secondproof/complete dispatch END;SIMULATED OS+physical/raw learned endpoints,NOT original model,full64,public driver,Work or TPS\n",execute?"RUN+119STOP+ALL120holds":"HELD no launch");
#ifdef PIONEER_Q38_RECURRENT_BLOCK_FINAL_RELEASE
#include "pioneer_q38_recurrent_block_native_final_checks.inc"
#endif
 io.recurrentTransport=nullptr;
#ifdef PIONEER_Q38_RECURRENT_BLOCK_ORIGINAL_NATIVE_TEST
 assert(original.Ready() && !f.cookie.context && !sRuntimeOwner && !sColdRecoveryRequired);
 if(stageSink && !transport.PublishOriginalStages(*stageSink))return 1;
 std::memcpy(output,originalCapture.data(),sizeof(originalCapture));return 0;
#endif
}
#endif
