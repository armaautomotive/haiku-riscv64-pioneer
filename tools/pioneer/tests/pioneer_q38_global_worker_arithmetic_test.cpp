/* SPDX-License-Identifier: MIT */
// SAME fresh QGW1 worker executes 64 actual down jobs then its 8/9 output
// waves. Necessary worker ordering ONLY, NOT all64 model/ROOT completion.
#define PIONEER_Q38_GLOBAL_WORKER_INITIAL_MACHINE_ONLY
#include "pioneer_q38_global_worker_initial_test.cpp"
#include "../PioneerQ38GlobalOutputPlan.h"
#include "pioneer_q38_global_output_original_source.h"
struct GlobalArithmeticPeer:GlobalInitialPeer {
 using GlobalInitialPeer::GlobalInitialPeer;
 unsigned partialStores=0,partials=0,downJobs=0,outputJobs=0;bool tamperAtPartial=false;
 void Store(uint32_t a,uint32_t value)override{
  if(Range(a,G::Partial,256)){
   assert(Range(pc,M::Code,unsigned(front.size())) && !pending && drained);
   unsigned n=partialStores,index=n<64?n:n<80?n-64:n<96?((n-80)&1?50+(n-80)/2:16+(n-80)/2):n<106?24+n-96:n<122?34+n-106:0;
   assert(n<=122 && a==G::Partial+index*4);
   if(n==122){assert(value==G::PartialMagic);partials++;}
   partialStores++;memory[a]=value;
   if(n==122 && tamperAtPartial)memory[M::Job+63*4]^=1;
   return;
  }
  GlobalInitialPeer::Store(a,value);
 }
 void Prepare(const std::array<uint32_t,64>& j,GlobalOutputOriginalSource* model=nullptr){
  assert(pc==waitPC && !pending && GdnFinished(sequence));ClearTensorState();
  assert(G::JobShape(j.data(),boot.data(),sequence,0) && j[1]==sequence+1 && (j[2]==1 || j[2]==G::GlobalMatrix));
  // Retire the endpoint's previous packet list while retaining its exact
  // cumulative physical counters. No PC/register/Tensix state is reset.
  uint32_t physical[8];for(unsigned i=0;i<8;i++)physical[i]=Count(i);
  for(unsigned i=0;i<8;i++)countBase[i]=physical[i];requests=responses=matrixInstructions=0;doorbellRead=false;
  denseActive=outputActive=gdnActive=convActive=activation=false;
  type=j[17];inputs=j[21];FfnPersistentPeer::groups=j[23];base=j[22];count=256;
  assert((type==12 || type==14) && (inputs==17408 || inputs==5120));
  packets.clear();root.clear();vector.resize(inputs);scales.resize(inputs/256);
  unsigned salt=j[5]*13+j[27]*71+logicalWorker*3;
  for(unsigned i=0;i<inputs;i++)vector[i]=PioneerInt8Operand(int((i*37+salt*17)%255)-127);
  for(unsigned i=0;i<inputs/2;i++)root[0xa0000+i*4]=unsigned(vector[i*2])|(unsigned(vector[i*2+1])<<16);
  unsigned scaleBytes=(unsigned(scales.size()*4)+63)&~63U;
  for(unsigned i=0;i<scales.size();i++)scales[i]=Bits(.0005f+float((i+salt)%7)*.0001f);
  for(unsigned i=0;i<scaleBytes/4;i++)root[0xab000+i*4]=i<scales.size()?scales[i]:0;
  for(unsigned at=0;at<inputs*2;at+=16384)packets.push_back({0xa0000+at,129,0xa0000+at,std::min(16384U,inputs*2-at),-1});
  packets.push_back({0xab000,129,0x94000,scaleBytes,-1});inputReads=unsigned(packets.size());
  tile.assign(PioneerQ38PanelTileBytes(type),0);tiles.assign(FfnPersistentPeer::groups*(inputs/256),tile);
  for(unsigned t=0;t<tiles.size();t++){
   for(unsigned byte=0;byte<tile.size();byte++){
    unsigned index=byte/16,row=j[28]+t/(inputs/256)*16+byte%16;
    uint16_t half=uint16_t(0x0800+((row+salt)%4)*0x80);unsigned value;
    if(type==12)value=index<4?(half>>((index%2)*8))&255:index<16?1+(t+row+index+salt)%3:(t*17+row*19+index*37+salt)%256;
    else value=index<128?(t*17+row*19+index*37+salt)%256:index<192?(row&1?0x55:0xaa):index<208?uint8_t(row&2?-int(1+(t+index)%3):int(1+(t+index)%3)):index<210?(half>>((index-208)*8))&255:0;
    tiles[t][byte]=uint8_t(value);
   }
   packets.push_back({base+t*unsigned(tile.size()),j[24],0x90000,unsigned(tile.size()),int(t)});
  }
  if(model){
   assert(type==14 && inputs==5120 && model->Ready());
   for(unsigned t=0;t<tiles.size();t++)assert(model->Tile(j[28]+t/20*16,t%20,tiles[t].data(),unsigned(tiles[t].size())));
  }
  // Poison derived L1 scratch/output, not an arithmetic result or private
  // processor state. Every published result must be written by instructions.
  Poison();PoisonTail();for(unsigned i=0;i<4096;i++)buffers[4096+i]=0x7fc00000;
  for(unsigned i=0;i<64;i++)memory[G::Partial+i*4]=0x7fc00001;
  partialStores=0;sequence=j[1];for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=j[i];memory[M::Doorbell]=sequence;
 }
 bool FinishGlobal(bool stop=false){
  while(pc!=faultPC && !(stop?pc==stopPC:GdnFinished(sequence)))StepBlock();
  if(pc==faultPC){assert(memory.at(M::Status+8)==G::Fault);return false;}
  assert(!pending && drained && minStack>=0x6e000 && memory.at(M::Status+12)==G::Magic && !memory.at(M::Status+8));
  return true;
 }
 void InertFault(){
  assert(pc==faultPC && memory.at(M::Status+8)==G::Fault);
  unsigned reads=requests,math=matrixInstructions,stores=partialStores;
  for(unsigned i=0;i<2048;i++)StepBlock();assert(pc==faultPC && requests==reads && matrixInstructions==math && partialStores==stores);
 }
 void FaultBeforeWork(const std::array<uint32_t,64>& j){
  assert(pc==waitPC && !pending && GdnFinished(sequence));
  unsigned reads=requests,math=matrixInstructions,stores=partialStores;
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=j[i];memory[M::Doorbell]=j[1];
  while(pc!=faultPC && pc!=stopPC)StepBlock();
  assert(pc==faultPC && memory.at(M::Status+4)==sequence && requests==reads && matrixInstructions==math && partialStores==stores);InertFault();
 }
 void Matrix(const std::array<uint32_t,64>& j,GlobalOutputOriginalSource* model=nullptr){
  Prepare(j,model);auto frozen=root;assert(FinishGlobal() && root==frozen);
  assert(requests==packets.size() && responses==requests && !dotProbeReads && CorrectRows());
  assert(memory.at(0x83f40)==FfnPersistentPeer::groups*inputs/16 && memory.at(0x83f44)==sequence && memory.at(0x83f48)==logicalWorker);
  if(j[2]==1){assert(!partialStores);downJobs++;return;}
  assert(partialStores==123 && partials==outputJobs+1); // zero64 + h16 + counters/id16 + command10 + source16 + magic1
  uint32_t expected[64]={};unsigned best=0;float maximum=Float(buffers[4096]);
  for(unsigned i=1;i<FfnPersistentPeer::groups*16;i++){
   float value=Float(buffers[4096+i]);assert(isfinite(value));if(value>maximum){maximum=value;best=i;}
  }
  const unsigned h[]={G::PartialMagic,j[1],64,j[6],j[3],j[27],j[28],FfnPersistentPeer::groups*16,j[28]+best,buffers[4096+best],1,j[7],248320,5120,14,1};
  for(unsigned i=0;i<16;i++)expected[i]=h[i];
  for(unsigned i=0;i<8;i++){expected[16+i]=Count(i);expected[50+i]=boot[8+i];}
  for(unsigned i=0;i<10;i++)expected[24+i]=command[i];
  for(unsigned i=0;i<16;i++)expected[34+i]=source[i];
  for(unsigned i=0;i<64;i++)assert(memory.at(G::Partial+i*4)==expected[i]);
  outputJobs++;
  printf("PASS ACTUAL QGW1 worker%u wave%u/rows%u+%u/local maximum%u/ALL64 partial words independently checked/%s RAW weights/SAME private instruction history\n",logicalWorker,j[27],j[28],FfnPersistentPeer::groups*16,j[28]+best,model?"ORIGINAL":"synthetic");
 }
 void StopGlobal(unsigned position){
  std::array<uint32_t,64> j={};const unsigned h[]={G::Magic,sequence+1,unsigned(G::Stop),logicalWorker,selfRoute,64,position,9999};
  for(unsigned i=0;i<8;i++){j[i]=h[i];j[8+i]=boot[8+i];}
  assert(G::JobShape(j.data(),boot.data(),sequence,0));sequence++;
  for(unsigned i=0;i<64;i++)memory[M::Job+i*4]=j[i];memory[M::Doorbell]=sequence;
  unsigned reads=requests,math=matrixInstructions;assert(FinishGlobal(true) && memory.at(M::Status+16)==2);
  for(unsigned i=0;i<2048;i++)StepBlock();assert(pc==stopPC && requests==reads && matrixInstructions==math);
 }
};
#ifndef PIONEER_Q38_GLOBAL_WORKER_ARITHMETIC_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || argc==4);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::unique_ptr<GlobalOutputOriginalSource> original;
 if(argc==4){
  puts("START authenticate full SAME open public c600 GGUF BEFORE any worker exists");
  original=std::make_unique<GlobalOutputOriginalSource>();assert(original->Open(argv[3]) && original->Ready());
  puts("PASS ORIGINAL c600de03 FULL SHA/same open file identity/all851 tensor schema/output Q6_K5120x248320;raw bytes ONLY,NO completed producer or native grant");
 }
 unsigned checked=0,stack=0;
 unsigned refusals=0,lateFaults=0,nextPositions=0;
 for(unsigned worker:{1U,21U,22U,119U}){
  puts("START QGW1 standalone worker arithmetic/64 actual down jobs are NECESSARY ordering ONLY,NOT all model operators or ROOT producer");
  auto m=std::make_unique<GlobalArithmeticPeer>(argv[1],argv[2],worker);m->BootGlobal();
  std::array<uint32_t,1920> source={};for(unsigned i=0;i<source.size();i++)source[i]=0x76540000+i+1;
  for(unsigned i=0;i<16;i++)source[worker*16+i]=m->source[i];
  PioneerQ38GdnIdentity id={1,2,3,4};auto output=std::make_unique<PioneerQ38GlobalOutputPlan>();
  assert(output->Build(*canonical,0,id,source.data()));std::array<uint32_t,64> boot;
  assert(output->WorkerBoot(worker,boot.data()) && boot==m->boot);
  for(unsigned layer=0;layer<64;layer++){
   auto j=m->Command(false);j[1]=m->sequence+1;j[3]=worker;j[4]=m->selfRoute;j[5]=layer;
   m->Matrix(j);if((layer&15)==15)printf("PASS ACTUAL QGW1 worker%u genuine down frontier%u/NO other model operators claimed\n",worker,layer+1);
  }
  std::array<uint32_t,64> first;assert(output->Job(0,worker,m->sequence,first.data()));
  for(unsigned bad=0;bad<8;bad++){
   if(original)assert(original->PrepareFork());
   fflush(nullptr);pid_t pid=fork();assert(pid>=0);
   if(!pid){
    auto j=first;
    if(bad==0)j[27]=1;if(bad==1)j[6]=1;
    if(bad==2){for(unsigned i=16;i<64;i++)j[i]=0;j[2]=G::Stop;}
    if(bad==3){j=m->Command(false);j[1]=m->sequence+1;j[5]=63;}
    if(bad==4)m->memory[M::Boot+40*4]^=1;if(bad==5)m->source[11]^=1;
    if(bad==6)m->countBase[4]++;if(bad==7)j[1]++;
    m->FaultBeforeWork(j);_exit(0);
   }
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));refusals++;
  }
  {
   if(original)assert(original->PrepareFork());
   fflush(nullptr);pid_t pid=fork();assert(pid>=0);
   if(!pid){
    unsigned previous=m->sequence;m->Prepare(first,original.get());m->tamperAtPartial=true;
    assert(!m->FinishGlobal() && m->partials==1 && m->partialStores==123 && m->requests==m->packets.size()
     && m->CorrectRows() && m->memory.at(M::Status+4)==previous);m->InertFault();_exit(0);
   }
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));lateFaults++;
  }
  unsigned waves=worker<=21?9:8;
  for(unsigned wave=0;wave<waves;wave++){
   std::array<uint32_t,64> j;assert(output->Job(wave,worker,m->sequence,j.data()));
   m->Matrix(j,original.get());checked++;
  }
  assert(m->downJobs==64 && m->outputJobs==waves);
  {
   if(original)assert(original->PrepareFork());
   fflush(nullptr);pid_t pid=fork();assert(pid>=0);
   if(!pid){
    auto j=m->Command(false);j[1]=m->sequence+1;j[6]=1;m->Matrix(j);
    assert(m->memory.at(M::Status+24)==0 && m->memory.at(M::Status+28)==1);
    j=first;j[1]=m->sequence+1;j[6]=1;m->FaultBeforeWork(j);_exit(0);
   }
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));nextPositions++;
  }
  m->StopGlobal(0);stack=std::max(stack,0x70000-m->minStack);
 }
 if(original)assert(original->Ready());
 printf("PASS ACTUAL QGW1 standalone4 workers/256 genuine down jobs/%u canonical output jobs+independent matrix and partial checks/%u frontier refusals/%u post-partial seal faults/%u genuine next-position first-down+premature-output refusals/8+9-wave boundaries/REAL global STOP/inert2048/written stack%u/8192;NO all64 model,normalized producer,ROOT,native,physical,multi-token-state,Work or TPS proof\n",checked,refusals,lateFaults,nextPositions,stack);
}
#endif
