/* SPDX-License-Identifier: MIT */
// Compare the actual linked ROOT-L1 services,not the GDDR-input diagnostic.
#define PIONEER_CACHED_FPU_IMAGE_ONLY
#include "pioneer_q38_cached_fpu_image_test.cpp"
struct TracedCachedMatrix:CachedMatrixRV32 {
 std::vector<uint64_t> stream;
 TracedCachedMatrix(const char* directory,CachedPipeline& engine,const CL::Job& job,const std::vector<uint8_t>& root)
  :CachedMatrixRV32(directory,engine,job,root){}
 void Instruction(unsigned word)override{stream.push_back(word);CachedMatrixRV32::Instruction(word);}
 void Store(unsigned address,unsigned value)override{
  if(Range(address,0xffef0000,224*4))stream.push_back((UINT64_C(1)<<63)|(uint64_t(address-0xffef0000)<<32)|value);
  CachedMatrixRV32::Store(address,value);
 }
};
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);unsigned cases=0,words=0;
 for(unsigned width:{5120U,17408U})for(unsigned groups:{2U,4U,16U}){
  const auto job=Job(width,groups,groups==16?119:groups==4?59:1,groups==4?1361:786);
  CachedData data(width,groups,0);std::vector<uint8_t> rootBytes(0x180000,0xa7);
  auto root=std::make_unique<ServiceMachine>(argv[1],rootBytes);
  const unsigned frame[]={PioneerQ38Bf16Input::Magic,1,width,0xb0000,0xa0000,17,0,0,0,0,0,0,0,0,0,0};
  for(unsigned i=0;i<16;i++)root->Put(0x74000+i*4,frame[i]);data.StageFp32(*root);
  assert(root->Execute(PioneerQ38Bf16Input::Magic));
  auto oldEngine=std::make_unique<CachedPipeline>();auto newEngine=std::make_unique<CachedPipeline>();
  std::fill(oldEngine->l1.begin(),oldEngine->l1.end(),0xa7);std::fill(newEngine->l1.begin(),newEngine->l1.end(),0xa7);
  auto old=std::make_unique<TracedCachedMatrix>(argv[2],*oldEngine,job,rootBytes);
  auto next=std::make_unique<TracedCachedMatrix>(argv[3],*newEngine,job,rootBytes);
  for(auto* machine:{old.get(),next.get()}){
   for(unsigned i=0;i<16;i++)machine->Put(0x74000+i*4,job.words[i]);data.StageCache(machine->bus);
   assert(machine->Execute(CL::Magic));
   assert(machine->Get(0x17ffc4)==groups*(width/16) && machine->forcedZero==272 && machine->stores==groups*16);
   for(unsigned pair=0;pair<groups/2;pair++)for(unsigned row=0;row<32;row++)
    assert(machine->Get(CL::Output+(pair*32+row)*4)==data.expected[pair][row]);
  }
  assert(old->stream==next->stream && next->steps<old->steps);
  assert(old->bus.issues==next->bus.issues && old->bus.completed==next->bus.completed);
  assert(oldEngine->Clean() && newEngine->Clean());
  printf("PASS_PAIRED_CACHED_ROOT_L1 width%u rows%u worker%u sameIssuedConfigStream%zu oldRV32%u pairedRV32%u all%uOutputsExact oldStack%u pairedStack%u;COUNTS_NOT_CARD_TIME_OR_MODEL_TPS\n",
   width,groups*16,job.words[6],next->stream.size(),old->steps,next->steps,groups*16,0x70000-old->minSP,0x70000-next->minSP);
  cases++;words+=groups*16;
 }
 assert(cases==6 && words==704);
 puts("PASS_PAIRED_CACHED_ROOT_L1 sixActualLinkedCases outputs704 exactTensixConfigStream unchangedPackets_noScalarWeights;NOT_NATIVE_HARDWARE_OR_MODEL_TPS");
}
