/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_cached_fpu_machine.h"
struct CachedData {
 std::vector<std::vector<Q4PipelineFixture>> pairs;std::vector<std::array<uint32_t,32>> expected;
 CachedData(unsigned width,unsigned groups,unsigned trial){
  for(unsigned pair=0;pair<groups/2;pair++){
   pairs.emplace_back();expected.emplace_back();expected.back().fill(0);
   for(unsigned block=0;block<width/256;block++)pairs.back().emplace_back(trial+pair*73+block);
  }
  for(unsigned block=0;block<width/256;block++)for(unsigned c=0;c<256;c++){
   unsigned high=pairs[0][block].input[c],tail=(c+block)%4==0?0:(c+block)%4==1?32767:(c+block)%4==2?32768:65535;
   high+=unsigned(tail>32768 || (tail==32768 && (high&1)));
   for(auto& pair:pairs)pair[block].input[c]=uint16_t(high);
  }
  for(unsigned p=0;p<pairs.size();p++)for(const auto& block:pairs[p])for(unsigned row=0;row<32;row++){
   float weights[256];assert(PioneerDecodeQ4K(block.original.data()+row*144,weights));
   for(unsigned first=0;first<256;first+=16){
    uint16_t a[16],b[16];for(unsigned c=0;c<16;c++){a[c]=uint16_t(Round(Q4PipelineFixture::Bits(weights[first+c]))>>16);b[c]=block.input[first+c];}
    for(unsigned phase=0;phase<4;phase++)assert(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,expected[p][row],expected[p][row]));
   }
  }
 }
 void StageCache(CachedIO& io)const{
  unsigned base=io.job.words[3]-CL::DramWeight,blocks=io.width/256;
  for(unsigned pair=0;pair<pairs.size();pair++)for(unsigned block=0;block<blocks;block++)for(unsigned row=0;row<32;row++)for(unsigned byte=0;byte<144;byte++)
   io.dram[base+((pair*2+row/16)*blocks+block)*2304+byte*16+row%16]=pairs[pair][block].original[row*144+byte];
 }
 void StageFp32(ServiceMachine& root)const{
  for(unsigned block=0;block<pairs[0].size();block++)for(unsigned c=0;c<256;c++){
   const unsigned highOriginal=uint16_t(0x3e80+(c*19+block*11)%384+((c&3)==2?0x8000:0));
   const unsigned tail=(c+block)%4==0?0:(c+block)%4==1?32767:(c+block)%4==2?32768:65535;
   root.Put(0xb0000+(block*256+c)*4,(highOriginal<<16)|tail);
  }
 }
};
static CL::Job Job(unsigned k,unsigned groups,unsigned worker,unsigned route){return {{CL::Magic,1,k,0x04004000,groups,route,worker,CL::Peer(worker),PioneerQ38Bf16Input::Magic,CL::InputBase,CL::InputRoot,17,CL::Output,0,0,0}};}
#ifndef PIONEER_CACHED_FPU_IMAGE_ONLY
int main(int argc,char** argv){
 assert(argc==3);unsigned cases=0,words=0;
 for(unsigned width:{5120U,17408U})for(unsigned groups:{2U,4U,16U}){
  const auto job=Job(width,groups,groups==16?119:groups==4?59:1,groups==4?1361:786);
  CachedData data(width,groups,0);std::vector<uint8_t> rootBytes(0x180000,0xa7);
  auto root=std::make_unique<ServiceMachine>(argv[1],rootBytes);
  const unsigned frame[]={PioneerQ38Bf16Input::Magic,1,width,0xb0000,0xa0000,17,0,0,0,0,0,0,0,0,0,0};
  for(unsigned i=0;i<16;i++)root->Put(0x74000+i*4,frame[i]);data.StageFp32(*root);
  assert(root->Execute(PioneerQ38Bf16Input::Magic) && root->Get(0x17ffc4)==width && root->stores==width/2);
  for(unsigned block=0;block<width/256;block++)for(unsigned c=0;c<256;c++){
   const unsigned at=0xa0000+(block*256+c)*2,value=rootBytes[at]|unsigned(rootBytes[at+1])<<8;
   assert(value==data.pairs[0][block].input[c]);
   if(groups==2 && block==0 && c<64)printf("ACTUAL_BF16_PRODUCER bits%08x value%04x\n",root->Get(0xb0000+c*4),value);
  }
  auto native=std::make_unique<CachedPipeline>();std::fill(native->l1.begin(),native->l1.end(),0xa7);
  auto matrix=std::make_unique<CachedMatrixRV32>(argv[2],*native,job,rootBytes);
  for(unsigned i=0;i<16;i++)matrix->Put(0x74000+i*4,job.words[i]);data.StageCache(matrix->bus);
  assert(matrix->Execute(CL::Magic));
  assert(matrix->Get(0x17ffc4)==groups*(width/16) && matrix->stores==groups*16 && matrix->forcedZero==272);
  assert(native->Clean() && native->outputReads==groups*16 && native->zeroLoads==5*(groups/2) && native->extraZeroLoads==7*(groups/2));
  assert(native->compactImports==groups*(width/256) && native->matrixOps==(groups/2)*(width/256)*128);
  assert(matrix->bus.jobs==groups/2 && matrix->bus.issues==groups/2*(width/256)*26 && matrix->bus.completed==matrix->bus.issues
   && matrix->bus.queued.empty() && matrix->bus.responses.empty() && matrix->bus.overlap);
  for(unsigned pair=0;pair<groups/2;pair++)for(unsigned row=0;row<32;row++){
   assert(matrix->Get(CL::Output+(pair*32+row)*4)==data.expected[pair][row]);words++;
  }
  printf("PASS actual cached-FPU services K%u rows%u worker%u:FP32-to-BF16 ROOT RV32 producer -> actual peer NoC -> existing16-row cache/fullK FPU returning matrix;all%u outputs exact,one clear/pair,SP%u/%u;NO scalar weight copy/native/lifetime/full-model/quality/speed proof\n",
   width,groups*16,job.words[6],groups*16,0x70000-root->minSP,0x70000-matrix->minSP);fflush(stdout);cases++;
 }
 assert(cases==6 && words==704);
 printf("PASS actual compiled cached-FPU matrix/producer %u cases/%u outputs;strict unchanged RV32/canonical L1/two real source routes/old cache bytes;NOT native/hardware/full-model quality/performance proof\n",cases,words);
}
#endif
