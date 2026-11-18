/* SPDX-License-Identifier: MIT */
// Observe exact returning vector92 code. Local arithmetic/transport simulation,
// not a device cycle counter. No changes to the worker instruction stream.
#define PIONEER_Q38_VECTOR_METADATA_WORKER_MACHINE_ONLY
#include "pioneer_q38_vector_q4_worker_test.cpp"

namespace {
enum Class {Metadata, MatrixIssue, Unpack, Vector, Replay, Sync, Other, Classes};
struct Report {
 std::array<unsigned,Classes> submitted={};
 unsigned instructions=0, configs=0, configReads=0, dynamicPort=0, nocReads=0, nocWrites=0, ackReads=0;
 unsigned inside=0, reads=0, expansions=0;
};

struct TracePeer:VectorMetadataPeer {
 const unsigned compute;
 unsigned returning=0,depth=0,begin=0,readBegin=0,expansionBegin=0;
 std::array<Report,2> reports={};
 unsigned finished=0;
 TracePeer(const char* legacy,const char* image,unsigned entry,unsigned format,unsigned k,unsigned groups)
  :VectorMetadataPeer(legacy,image,format,k,groups),compute(entry){}
 Report& Current(){assert(finished<reports.size());return reports[finished];}
 void StepBlock()override {
  if(pc==compute){
   assert(matrixScope && !returning && finished<reports.size());
   returning=regs[1];begin=steps;readBegin=requests;expansionBegin=replayExpansions;
  }
  const bool in=returning;
  const bool meta=in && (active || pc==metadataPC);
  const unsigned prior=steps;
  VectorMetadataPeer::StepBlock();
  if(in){
   assert(steps==prior+1); // no skipped instructions or artificial endpoint stalls in the observed scope
   if(meta)Current().inside++;
   if(pc==returning){
    auto& report=Current();report.instructions=steps-begin;report.reads=requests-readBegin;
    report.expansions=replayExpansions-expansionBegin;
    assert(!active && !depth && report.instructions && report.submitted[MatrixIssue] && report.reads);
    returning=0;finished++;
   }
  }
 }
 void Instruction(uint32_t op)override {
  // Lower observers may replay nested semantic work. Count actual submitted
  // commands once; separately account for the observer's replay expansions.
  if(returning && !depth){
   const unsigned code=op>>24;
   const auto kind=active?Metadata:code==0x26?MatrixIssue:code==0x42 || code==0x43?Unpack:
    code==0x04?Replay:op==0xa2101816 || op==0x4500017e?Sync:code>=0x70 && code<=0x90?Vector:Other;
   Current().submitted[kind]++;
  }
  depth++;
  VectorMetadataPeer::Instruction(op);
  depth--;
 }
 uint32_t Load(uint32_t at)override {
  if(returning){
   auto& r=Current();r.configReads+=Range(at,0xffef0000,224*4);
   r.nocReads+=Range(at,0xffb20000,0x2000);
   r.ackReads+=at==0xffe000fc || at==0xffe80004;
  }
  return VectorMetadataPeer::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override {
  if(returning){
   auto& r=Current();r.configs+=Range(at,0xffef0000,224*4);
   r.nocWrites+=Range(at,0xffb20000,0x2000);r.dynamicPort+=at==0xffe40000;
  }
  VectorMetadataPeer::Store(at,value);
 }
};
}

int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 const unsigned entry=unsigned(std::stoul(argv[3],nullptr,16));
 unsigned cases=0;
 for(unsigned format:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U}){
  auto p=std::make_unique<TracePeer>(argv[1],argv[2],entry,format,k,groups);
  p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();p->AwaitPayload();p->FinishPushed(true);p->CheckEnd();
  std::array<uint32_t,16> first;
  for(unsigned i=0;i<16;i++)first[i]=p->buffers[4096+i];
  p->ReceiveQueued();p->AwaitSecond();p->FinishSecond(first);
  assert(p->finished==2 && !p->depth && !p->returning);
  assert(p->metadataBlocks==(format==12?2*groups*(k/256):0));
  for(unsigned job=0;job<2;job++){
   const auto& r=p->reports[job];
   assert(bool(r.inside)==(format==12) && bool(r.submitted[Metadata])==(format==12));
   assert(r.submitted[MatrixIssue]==p->ExpectedMatrixInstructions());
   unsigned total=0;for(unsigned n:r.submitted)total+=n;
   assert(total && r.dynamicPort<=total && r.submitted[MatrixIssue]<total);
   printf("VECTOR92_ISSUE format%u K%u groups%u job%u RV%u metadataRV%u submitted%u "
    "metadata%u matrix%u unpack%u sfpuOutsideMetadata%u replay%u sync%u other%u "
    "dynamic%u configReads%u configWrites%u ackReads%u NoCRegsRead%u NoCRegsWrite%u "
    "NoCReadRequests%u replayExpansions%u\n",format,k,groups,job+1,r.instructions,r.inside,total,
    r.submitted[Metadata],r.submitted[MatrixIssue],r.submitted[Unpack],r.submitted[Vector],r.submitted[Replay],
    r.submitted[Sync],r.submitted[Other],r.dynamicPort,r.configReads,r.configs,r.ackReads,r.nocReads,r.nocWrites,r.reads,r.expansions);
  }
  cases++;
 }
 assert(cases==18);
 printf("PASS_VECTOR92_ISSUE 18 shapes/36 complete retained jobs; actual linked instructions and checked outputs; synthetic NoC, NOT device cycles or TPS\n");
}
