/* SPDX-License-Identifier: MIT */
// OFFBOARD instruction counts from the actual compiled worker,NOT cycles/TPS.
#define PIONEER_Q38_PREPARED_WORKER_MACHINE_ONLY
#include "pioneer_q38_prepared_exact_q4_worker_test.cpp"
#include <algorithm>
#include <cstdint>
#if !defined(PIONEER_Q38_TWO_PRODUCT_REPLAY_WORKER_TEST) || PIONEER_Q38_TWO_PRODUCT_REPLAY_WORKER_TEST != 1
#error Explicit replay13 OFFBOARD instruction-count diagnostic only
#endif
struct InstructionProfilePeer:PreparedQ4Peer {
 using PreparedQ4Peer::PreparedQ4Peer;
 struct Count{uint64_t retired=0,kernel=0,observed=0;unsigned word=0;};
 std::map<unsigned,Count> profile;
 uint64_t observed=0,retired=0,unretired=0,kernelRetired=0;
 enum {L1,Stack,Config,DrainAck,Noc,Output,InstructionPort,OtherMmio,Classes};
 uint64_t loads[Classes]={},stores[Classes]={};
 static unsigned MemoryClass(unsigned at){
  if(Range(at,0xffef0000,224*4))return Config;
  if(at==0xffe000fc)return DrainAck;
  if(Range(at,0xffb20000,68))return Noc;
  if(Range(at,0xffbd9b00,64))return Output;
  if(at==0xffe40000)return InstructionPort;
  if(at>=0xff000000)return OtherMmio;
  return Range(at,0x6e000,0x2000)?Stack:L1;
 }
 unsigned Load(unsigned at)override{if(exact)loads[MemoryClass(at)]++;return PreparedQ4Peer::Load(at);}
 void Store(unsigned at,unsigned value)override{if(exact)stores[MemoryClass(at)]++;PreparedQ4Peer::Store(at,value);}
 void StepBlock()override{
  const unsigned at=pc,word=l1[Index(at)],before=steps;
  const bool kernel=exact || at==exactPC;
  PreparedQ4Peer::StepBlock();
  // Some fixture interception paths observe a step without retiring an RV
  // instruction. Keep these separate;never invent guest instruction costs.
  assert(steps==before || steps==before+1);
  auto& c=profile[at];if(c.observed)assert(c.word==word);c.word=word;c.observed++;
  observed++;if(steps==before+1){c.retired++;retired++;if(kernel){c.kernel++;kernelRetired++;}}else unretired++;
 }
 void Report(const char* image)const{
  struct Function{unsigned first,size;std::string name;};
  std::vector<Function> functions;std::ifstream in(std::string(image)+"/program.functions");
  assert(in);std::string name,kind,first,size;
  while(in>>name>>kind>>first>>size){
   assert(kind=="FUNC");functions.push_back({unsigned(std::stoul(first,nullptr,16)),unsigned(std::stoul(size,nullptr,10)),name});
  }
  assert(!functions.empty());std::map<std::string,uint64_t> totals,kernelTotals;
  uint64_t opcodes[128]={},sum=0;
  for(const auto& [at,c]:profile){
   if(!c.retired)continue;
   const Function* match=nullptr;
   for(const auto& f:functions)if(at>=f.first && at-f.first<f.size){
    if(match){assert(match->first==f.first && match->size==f.size);if(f.name<match->name)match=&f;}
    else match=&f;
   }
   totals[match?match->name:"UNATTRIBUTED"]+=c.retired;
   kernelTotals[match?match->name:"UNATTRIBUTED"]+=c.kernel;
   opcodes[c.word&127]+=c.retired;sum+=c.retired;
  }
  assert(sum==retired && retired==steps && observed==retired+unretired);
  printf("RV_PROFILE K%u groups%u jobs%u retired%llu exact_kernel_retired%llu observed%llu intercepted_without_retirement%llu;SYNTHETIC_NOT_CYCLES_OR_TPS\n",
   inputs,FfnPersistentPeer::groups,exactJobs,(unsigned long long)retired,(unsigned long long)kernelRetired,(unsigned long long)observed,(unsigned long long)unretired);
  std::vector<std::pair<uint64_t,std::string>> ranked;
  for(const auto& [function,count]:totals)ranked.push_back({count,function});
  std::sort(ranked.rbegin(),ranked.rend());
  for(const auto& [count,function]:ranked)printf("RV_FUNCTION %llu %s\n",(unsigned long long)count,function.c_str());
  uint64_t kernelSum=0;ranked.clear();
  for(const auto& [function,count]:kernelTotals)if(count){ranked.push_back({count,function});kernelSum+=count;}
  assert(kernelSum==kernelRetired);std::sort(ranked.rbegin(),ranked.rend());
  for(const auto& [count,function]:ranked)printf("RV_KERNEL_FUNCTION %llu %s\n",(unsigned long long)count,function.c_str());
  const char* const classes[]={"local_L1","stack","config_register","drain_ack","noc_register","output_register","instruction_port","other_mmio"};
  static_assert(sizeof(classes)/sizeof(classes[0])==Classes,"all memory callbacks classified");
  for(unsigned i=0;i<Classes;i++)printf("RV_KERNEL_MEMORY %s loads%llu stores%llu;CALLBACK_COUNTS_NOT_LATENCY\n",classes[i],(unsigned long long)loads[i],(unsigned long long)stores[i]);
  for(unsigned op=0;op<128;op++)if(opcodes[op])printf("RV_OPCODE %02x %llu\n",op,(unsigned long long)opcodes[op]);
  std::vector<std::pair<uint64_t,unsigned>> hottest;
  for(const auto& [at,c]:profile)if(c.retired)hottest.push_back({c.retired,at});
  std::sort(hottest.rbegin(),hottest.rend());
  for(unsigned i=0;i<std::min(size_t(24),hottest.size());i++){
   const auto [count,at]=hottest[i];printf("RV_HOT_PC %08x word%08x retired%llu\n",at,profile.at(at).word,(unsigned long long)count);
  }
 }
};
template<class Peer>static void TwoJobs(Peer& p){
 p.delay=3;p.BootPushed();p.Prepare();p.StagePushed();p.AwaitPayload();p.FinishPushed(true);p.CheckEnd();
 std::array<uint32_t,16> first{};for(unsigned i=0;i<16;i++)first[i]=p.buffers[4096+i];
 p.ReceivePreparedQueued();p.AwaitSecond();p.FinishSecond(first);p.CheckEnd();
 assert(p.exactJobs==2 && p.pc==p.stopPC && !p.pending && !p.heldPayload);
}
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 for(unsigned k:{5120U,6144U,17408U})for(unsigned groups:{1U,3U}){
  auto control=std::make_unique<PreparedQ4Peer>(argv[1],argv[3],12,k,groups);
  auto measured=std::make_unique<InstructionProfilePeer>(argv[1],argv[3],12,k,groups);
  TwoJobs(*control);TwoJobs(*measured);
  assert(control->steps==measured->steps && control->requests==measured->requests
   && control->responses==measured->responses && control->mailbox==measured->mailbox
   && control->memory==measured->memory && control->affineReplayRuns==measured->affineReplayRuns);
  for(unsigned r=0;r<groups*16;r++)assert(control->buffers[4096+r]==measured->buffers[4096+r]);
  measured->Report(argv[3]);
 }
 puts("PASS_ACTUAL_WORKER_INSTRUCTION_PROFILE six_geometries_two_changed_jobs_STOP exact_unobserved_control_steps_state_output;NOT_PHYSICAL_CYCLES_OR_TPS");
}
