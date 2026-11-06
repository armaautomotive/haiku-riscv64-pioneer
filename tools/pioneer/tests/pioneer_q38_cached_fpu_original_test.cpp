/* SPDX-License-Identifier: MIT */
// Separate CPU validation,never linked into the returning services.
#define PIONEER_CACHED_FPU_IMAGE_ONLY
#include "pioneer_q38_cached_fpu_image_test.cpp"
#include "../PioneerNativeCompactQ4StreamSource.h"
int main(int argc,char** argv){
 assert(argc==4);PioneerNativeCompactQ4StreamSource source;std::string error;
 assert(source.Prepare(argv[3],error) && source.Ready());
 using Data=PioneerNativeCompactQ4StreamData;using Meta=PioneerNativeCompactQ4StreamMetadata;
 for(unsigned sample=0;sample<9;sample++){
  const unsigned width=Data::Width(sample),blocks=width/256;
  const auto job=Job(width,2,1+sample*13,(sample&1)?1361:786);
  std::vector<uint8_t> rootBytes(0x180000,0xa7);auto root=std::make_unique<ServiceMachine>(argv[1],rootBytes);
  const unsigned frame[]={PioneerQ38Bf16Input::Magic,1,width,0xb0000,0xa0000,17,0,0,0,0,0,0,0,0,0,0};
  for(unsigned i=0;i<16;i++)root->Put(0x74000+i*4,frame[i]);
  for(unsigned b=0;b<blocks;b++)for(unsigned word=0;word<128;word++){
   const unsigned bits=Data::Word(sample,blocks*1152+b*128+word);
   root->Put(0xb0000+(b*256+word*2)*4,(bits&65535)<<16);root->Put(0xb0000+(b*256+word*2+1)*4,bits&0xffff0000);
  }
  assert(root->Execute(PioneerQ38Bf16Input::Magic) && root->Get(0x17ffc4)==width);
  for(unsigned b=0;b<blocks;b++)for(unsigned word=0;word<128;word++)assert(root->Get(0xa0000+b*512+word*4)==Data::Word(sample,blocks*1152+b*128+word));
  auto native=std::make_unique<CachedPipeline>();std::fill(native->l1.begin(),native->l1.end(),0xa7);
  auto matrix=std::make_unique<CachedMatrixRV32>(argv[2],*native,job,rootBytes);
  for(unsigned i=0;i<16;i++)matrix->Put(0x74000+i*4,job.words[i]);
  // Invert ONLY storage transposition into the existing16-row byte-major
  // cache. The actual source method above already bound every compiled
  // compact word/input/golden to the SAME-open fresh original GGUF.
  for(unsigned b=0;b<blocks;b++)for(unsigned half=0;half<2;half++)for(unsigned byte=0;byte<144;byte++)for(unsigned row=0;row<16;row++){
   const unsigned at=byte<16?4096+half*256+byte*16+row:half*2048+(byte-16)*16+row;
   const unsigned bits=Data::Word(sample,b*1152+at/4),value=(bits>>(at%4*8))&255;
   matrix->bus.dram[job.words[3]-CL::DramWeight+(half*blocks+b)*2304+byte*16+row]=uint8_t(value);
  }
  assert(source.Ready() && matrix->Execute(CL::Magic) && matrix->Get(0x17ffc4)==2*(width/16));
  for(unsigned row=0;row<32;row++)assert(matrix->Get(CL::Output+row*4)==Meta::Golden(sample)[row]);
  assert(source.Ready() && native->Clean() && matrix->stores==32 && matrix->bus.jobs==1 && matrix->bus.issues==blocks*26);
  printf("PASS PRIVATE cached-FPU original sample%u tensor%u first%u K%u:actual ROOT BF16 producer/peer RV32 returning fullK/all32 original-bound words exact,existing16-row cache/two NoC source spaces;NO native/hardware/full-model-quality/speed proof\n",sample,Meta::Tensor(sample),Meta::First(sample),width);fflush(stdout);
 }
 puts("PASS PRIVATE cached-FPU original288 exact outputs/9 fullK bands/fresh full-original SHA and nanosecond FD identity;NOT native/producer-lifetime/full-model inference/quality/tokens-sec authority");
}
