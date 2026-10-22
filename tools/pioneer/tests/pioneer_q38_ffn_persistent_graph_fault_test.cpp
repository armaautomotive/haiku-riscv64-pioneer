/* SPDX-License-Identifier: MIT */
// New persistent owner policy faults. Frozen zero+119 boots are ACTUAL
// instructions;faults cover pre-arithmetic and physical STOP proof,not
// full FFN numeric corruption or native execution.
// Endpoint/cookie/clock scope remains simulated,NOT a native qualification.
#define PIONEER_Q38_PERSISTENT_GRAPH_FIXTURE_ONLY
#include "pioneer_q38_ffn_persistent_graph_test.cpp"
static void Retained(PersistentFixture& f){
 assert(f.graphOwner->Unsafe() && !f.graphOwner->Ready() && !f.graphOwner->Closed() && f.result.unsafe
  && f.code->Unsafe() && !f.code->Closed() && f.cohort->unsafe && f.banks.Quarantined()
  && f.mutableOwner->Unsafe() && f.io.quarantined && f.io.retained && !f.result.closed);
 const unsigned touches=f.io.touches;std::array<uint32_t,5120> before;
 for(unsigned i=0;i<5120;i++)before[i]=f.output[i]=0xdeadbeef;
 assert(!f.Prepare() && !f.Run() && !f.graphOwner->CopyOutput(f.io,f.output,f.result) && !f.Close() && touches==f.io.touches);
 for(unsigned i=0;i<5120;i++)assert(f.output[i]==before[i]);
 for(auto& n:f.cohort->worker)assert(n.image.unsafe && n.Store().recovery_required);
}
int main(int argc,char** argv){
 assert(argc==4);GraphImage=argv[1];PeerImage=argv[2];setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&PreparePanel;(void)&Run;(void)&Job;(void)&ClosePanel;(void)&Finish;(void)&FreshInputs;
 unsigned refusal=0,faults=0,stopFaults=0;
 for(unsigned selected:{0U,1U,60U,119U})for(unsigned kind:{1U,3U,4U,5U,6U,7U}){
  if(!selected && kind==1)continue; // root boot is installed before the worker fault target
  PersistentFixture f(argv[3]);f.io.graphFault=kind;f.io.faultWorker=selected;
  assert(!f.Prepare() && f.io.observed==f.io.touches);Retained(f);faults++;
 }
 for(unsigned kind:{3U,4U,5U,6U,7U}){
  PersistentFixture f(argv[3]);assert(f.Prepare());f.io.graphFault=kind;f.io.faultPhase=6;f.io.faultWorker=0;
  assert(!f.Run() && f.io.observed==f.io.touches && !f.io.rootCount && !f.result.rootStarts);Retained(f);faults++;
 }
 {PersistentFixture f(argv[3]);assert(f.Prepare());f.io.scope=false;unsigned before=f.io.touches;
  assert(!f.Run() && f.io.touches==before && !f.io.rootCount);Retained(f);faults++;}
 for(unsigned word:{0U,1U,2U,6U,7U,8U,9U,10U,11U,12U,16U,24U,32U,48U,49U,54U,55U,56U,57U,63U}){
  PersistentFixture f(argv[3]);assert(f.Prepare());f.io.graphFault=2;f.io.faultWord=word;
  assert(!f.Close() && f.io.observed==f.io.touches && f.io.rootCount==1 && !f.result.rootHolds
   && !f.result.held && !f.result.restoredWords);Retained(f);stopFaults++;
 }
 for(unsigned which=0;which<7;which++){
  PersistentFixture f(argv[3]);unsigned before=f.io.touches;
  if(which==0)f.id.modelEpoch++;if(which==1)f.id.driverGeneration++;
  if(which==2)f.result.prepared=1;if(which==3)f.result.runs=1;if(which==4)f.result.restoredWords=867448;
  if(which==5)f.result.rootStarts=1;if(which==6)f.result.closed=1;
  assert(!f.Prepare() && before==f.io.touches && !f.graphOwner->Unsafe() && !f.io.retained);refusal++;
 }
 {PersistentFixture f(argv[3]);assert(f.Prepare());auto second=std::make_unique<PioneerQ38FfnPersistentGraphOwner>();
  auto code=std::make_unique<PioneerQ38FfnGraphCodeOwner>();PioneerQ38FfnPersistentGraphResult r;unsigned before=f.io.touches;
  assert(!second->Prepare(f.io,f.proof,f.banks,*f.cohort,*code,*f.mutableOwner,*f.plan,f.id,f.learned.data(),unsigned(f.learned.size()),*f.jobs,0,0,r)
   && f.io.touches==before && !second->Unsafe() && f.mutableOwner->Ffn(f.id)->OwnsGraphLease(f.graphOwner.get()));refusal++;}
 {PersistentFixture f(argv[3]);assert(f.Prepare());unsigned before=f.io.touches;
  assert(!f.Prepare() && !f.graphOwner->CopyOutput(f.io,f.output,f.result) && f.io.touches==before);refusal++;
  f.values[255]=0x7fc00001;assert(!f.Run() && f.io.touches==before && f.graphOwner->Ready() && !f.io.rootCount);refusal++;
  f.values[255]=0x3f800000;assert(!f.Run(1,0) && f.io.touches==before && f.graphOwner->Ready());refusal++;
 }
 printf("PASS persistent FFN owner %u pre-arithmetic and %u actual ROOT-STOP proof fault-retentions/%u inert refusals:private graph claim excludes a second owner,119actualboots/one code lifetime,first rejected map/reset/source/PCI/install/scope/stat proof retains ALL with ZERO later IO/STOP/reset/restore/retry or firsthold,atomic caller output unchanged;SIMULATED_ENDPOINTS_NOT_NATIVE_OR_FULL_FFN_NUMERIC_EVIDENCE\n",faults,stopFaults,refusal);
}
