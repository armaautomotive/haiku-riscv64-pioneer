/* SPDX-License-Identifier: MIT */
#define PIONEER_CACHED_FPU_IMAGE_ONLY
#include "pioneer_q38_cached_fpu_image_test.cpp"
int main(int argc,char** argv){
 assert(argc==3);unsigned words=0,expectedIssues=0,expectedOps=0,expectedImports=0,expectedInputs=0;
 std::vector<uint8_t> rootBytes(0x180000,0xa7);
 auto root=std::make_unique<ServiceMachine>(argv[1],rootBytes);
 auto native=std::make_unique<CachedPipeline>();std::fill(native->l1.begin(),native->l1.end(),0xa7);
 auto first=Job(5120,4,59,786);
 auto matrix=std::make_unique<CachedMatrixRV32>(argv[2],*native,first,rootBytes);
 const unsigned widths[]={5120,17408,5120,17408},groups[]={4,2,16,4};
 for(unsigned trial=0;trial<4;trial++){
  auto job=Job(widths[trial],groups[trial],59,trial&1?1361:786);job.words[11]+=trial;
  CachedData data(job.words[2],job.words[4],0);
  const unsigned frame[]={PioneerQ38Bf16Input::Magic,1,job.words[2],0xb0000,0xa0000,17+trial,0,0,0,0,0,0,0,0,0,0};
  for(unsigned i=0;i<16;i++)root->Put(0x74000+i*4,frame[i]);data.StageFp32(*root);
  if(trial){root->pc=0x6000;for(unsigned i=0;i<3;i++)root->Put(0x17ffc0+i*4,0);}
  assert(root->Execute(PioneerQ38Bf16Input::Magic) && root->Get(0x17ffc4)==job.words[2]);
  if(trial)matrix->Reenter(job);else for(unsigned i=0;i<16;i++)matrix->Put(0x74000+i*4,job.words[i]);
  data.StageCache(matrix->bus);assert(matrix->Execute(CL::Magic));
  assert(matrix->Get(0x17ffc4)==job.words[4]*(job.words[2]/16) && native->Clean());
  for(unsigned pair=0;pair<job.words[4]/2;pair++)for(unsigned row=0;row<32;row++){
   assert(matrix->Get(CL::Output+(pair*32+row)*4)==data.expected[pair][row]);words++;
  }
  const unsigned blocks=job.words[2]/256,pairs=job.words[4]/2;
  expectedIssues+=pairs*blocks*26;expectedOps+=pairs*blocks*128;
  expectedImports+=pairs*blocks*2;expectedInputs+=pairs*blocks*8;
  assert(matrix->bus.issues==expectedIssues && matrix->bus.completed==expectedIssues && native->matrixOps==expectedOps
   && native->compactImports==expectedImports && native->inputLoads==expectedInputs && matrix->forcedZero==272*(trial+1)
   && matrix->stores==words && native->outputReads==words && matrix->regs[27]==0x13579bdf);
  printf("PASS WARM cached-FPU request%u K%u rows%u route%u:all%u outputs exact,same actual RV32/NoC/FPU/SFPU/L1/counters live,no engine reset\n",
   job.words[11],job.words[2],job.words[4]*16,job.words[5],job.words[4]*16);fflush(stdout);
 }
 assert(words==416);
 puts("PASS WARM cached-FPU4 alternating-width requests/416 exact outputs;caller changes source width/route without clearing live engine or counters;NOT actual persistent-worker/native/full-model/hardware/performance authority");
}
