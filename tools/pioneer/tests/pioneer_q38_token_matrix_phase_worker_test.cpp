/* SPDX-License-Identifier: MIT */
// Actual complete old/new worker instructions. Clock/NoC/raw endpoints are
// synthetic;clock DATA is reconstructed independently from guest CSR reads.
#define PIONEER_Q38_CONSTANT_ISSUE_WORKER_MACHINE_ONLY
#include "pioneer_q38_q4_constant_issue_worker_test.cpp"
struct MatrixPhasePeer:ConstantIssuePeer {
 using ConstantIssuePeer::ConstantIssuePeer;
 std::array<uint32_t,16> metric={{0xa5a5a5a5,0xa5a5a5a5,0xa5a5a5a5,0xa5a5a5a5,
  0xa5a5a5a5,0xa5a5a5a5,0xa5a5a5a5,0xa5a5a5a5,0xa5a5a5a5,0xa5a5a5a5,0xa5a5a5a5,0xa5a5a5a5,
  0xa5a5a5a5,0xa5a5a5a5,0xa5a5a5a5,0xa5a5a5a5}};
 unsigned metricWrites=0,matrixClockReads=0,matrixClockFault=0;
 uint64_t matrixClock=0;
 std::vector<uint64_t> samples;
 uint32_t ReadCycleCsr(unsigned csr)override{
  if(!matrixScope)return ConstantIssuePeer::ReadCycleCsr(csr);
  assert(type==12 && (csr==0xb00 || csr==0xb80));unsigned at=matrixClockReads++;
  if(at%3==0){
   matrixClock=UINT64_C(0x10000000000)+uint64_t(steps)*19;
   if(matrixClockFault==2 && at>=3)matrixClock-=UINT64_C(1)<<39;
   if(matrixClockFault==3)matrixClock=UINT64_C(0x10000000000);
   samples.push_back(matrixClock);
  }
  uint32_t value=csr==0xb00?uint32_t(matrixClock):uint32_t(matrixClock>>32);
  if(matrixClockFault==1 && csr==0xb80)value+=at;
  return value;
 }
 uint32_t Load(uint32_t at)override{
  if(Range(at,0x84500,64)){assert(!(at&3));return metric[(at-0x84500)/4];}
  return ConstantIssuePeer::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  if(Range(at,0x84500,64)){
   assert(matrixScope && type==12 && !(at&3) && (at-0x84500)/4==metricWrites%16);
   metric[(at-0x84500)/4]=value;metricWrites++;return;
  }
  ConstantIssuePeer::Store(at,value);
 }
 void CheckMetric(unsigned jobs,bool valid){
  if(type!=12){
   assert(!matrixClockReads && !metricWrites && samples.empty());
   for(auto word:metric)assert(word==0xa5a5a5a5);return;
  }
  assert(metricWrites==jobs*16 && metric[0]==0x324d5051 && metric[1]==2
   && metric[2]==memory.at(0x74000+12*4) && metric[3]==69 && metric[4]==12
   && metric[5]==inputs && metric[6]==FfnPersistentPeer::groups && metric[7]==unsigned(valid)
   && !metric[14] && !metric[15]);
  const unsigned blocks=FfnPersistentPeer::groups*(inputs/256);
  assert(matrixClockReads==blocks*34*3*(matrixClockFault==1?4:1));
  if(!valid){for(unsigned i=8;i<16;i++)assert(!metric[i]);return;}
  assert(samples.size()==blocks*34);uint64_t totals[3]={};
  for(unsigned block=0;block<blocks;block++){
   const unsigned base=block*34;totals[0]+=samples[base+1]-samples[base];
   for(unsigned p=0;p<16;p++){
    totals[1]+=samples[base+2+p*2]-samples[base+1+p*2];
    totals[2]+=samples[base+3+p*2]-samples[base+2+p*2];
   }
  }
  for(unsigned b=0;b<3;b++)assert(((uint64_t(metric[9+b*2])<<32)|metric[8+b*2])==totals[b] && totals[b]);
 }
 void NextMetricObservation(){matrixClockReads=0;samples.clear();} // OBSERVER ONLY;guest state/counters retained
};
static void Equivalent(const MatrixPhasePeer& a,const MatrixPhasePeer& b){
 assert(a.issued==b.issued && a.matrixInstructions==b.matrixInstructions && a.rawReads==b.rawReads
  && a.rawUnpacks==b.rawUnpacks && a.drains==b.drains && a.vectorOps==b.vectorOps
  && a.replayRecords==b.replayRecords && a.replayExpansions==b.replayExpansions
  && a.requests==b.requests && a.responses==b.responses && a.sumReads==b.sumReads && a.sumWrites==b.sumWrites
  && a.affineRecords==b.affineRecords && a.affineExpansions==b.affineExpansions
  && a.baseWrites==b.baseWrites && a.copyWrites==b.copyWrites && !a.dotProbeReads && !b.dotProbeReads);
 for(unsigned row=0;row<a.FfnPersistentPeer::groups*16;row++)assert(a.buffers[4096+row]==b.buffers[4096+row]);
 for(unsigned i=0;i<64;i++)assert(a.mailbox[i]==b.mailbox[i]);
 assert(!a.matrixClockReads && !a.metricWrites);
}
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);(void)&Compare;(void)&RunPersistentGdnPair;
 unsigned cases=0,clockFaults=0,losses=0,stack=0;
 for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U,16U}){
  std::printf("MATRIX_PHASE_CASE type%u K%u groups%u\n",type,k,groups);
  auto old=std::make_unique<MatrixPhasePeer>(argv[1],argv[2],69,type,k,groups);
  auto next=std::make_unique<MatrixPhasePeer>(argv[1],argv[3],69,type,k,groups);
  for(unsigned side=0;side<2;side++){
   auto& p=side?*next:*old;p.delay=3;p.BootPushed();p.Prepare();p.StagePushed();p.AwaitPayload();p.FinishPushed(true);p.CheckEnd();
  }
  Equivalent(*old,*next);next->CheckMetric(1,true);
  std::array<uint32_t,16> first[2];
  for(unsigned side=0;side<2;side++){
   auto& p=side?*next:*old;for(unsigned i=0;i<16;i++)first[side][i]=p.buffers[4096+i];
   p.NextMetricObservation();p.ReceiveQueued();p.AwaitSecond();p.FinishSecond(first[side]);
   stack=std::max(stack,0x70000-p.minStack);
  }
  Equivalent(*old,*next);next->CheckMetric(2,true);cases++;
 }
 for(unsigned fault:{1U,2U,3U}){
  auto old=std::make_unique<MatrixPhasePeer>(argv[1],argv[2],69,12,5120,1);
  auto next=std::make_unique<MatrixPhasePeer>(argv[1],argv[3],69,12,5120,1);
  next->matrixClockFault=fault;
  for(unsigned side=0;side<2;side++){
   auto& p=side?*next:*old;p.delay=3;p.BootPushed();p.Prepare();p.StagePushed();p.AwaitPayload();p.FinishPushed(false);p.CheckEnd();
  }
  Equivalent(*old,*next);next->CheckMetric(1,false);clockFaults++;
 }
 for(unsigned type:{8U,12U,14U})for(unsigned which:{0U,1U,2U,3U}){
  auto p=std::make_unique<MatrixPhasePeer>(argv[1],argv[3],69,type,5120,3);
  p->delay=3;p->BootPushed();p->Prepare();p->StagePushed();
  p->faultAt=which==0?1:which==1?p->inputReads+1
   :which==2?p->inputReads+unsigned(p->tiles.size()/p->Batch()/2):unsigned(p->packets.size());
  p->ReadFault();losses++;
 }
 std::printf("PASS_MATRIX_PHASE_ACTUAL_WORKER shapes%u retainedChangedInputJobs%u invalidClockCompletesUnchanged%u NoCLossInert%u maxCombinedStack%u/8192;"
  "ALL original tensor streams/results/NoC/STOP preserved;metric totals reconstructed ONLY from actual guest reads of FABRICATED CSR clock;"
  "NO native owner,original model,physical timing,Work or TPS grant\n",cases,cases*2,clockFaults,losses,stack);
}
