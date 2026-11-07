/* SPDX-License-Identifier: MIT */
// Same pinned Qwen-derived input for both actual RV32 images. No GPU access.
#define PIONEER_Q38_RMS_MACHINE_ONLY
#include "pioneer_q38_learned_rms_image_test.cpp"
#include "../PioneerModelWeightCache.h"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static uint32_t Word(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
static bool ReadPinned(const char* path,size_t size,const char* digest,std::vector<uint8_t>& bytes){
 if(!path || path[0]!='/')return false;
 const int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return false;
 struct stat before={},after={};bytes.assign(size,0);size_t done=0;
 bool okay=!fstat(fd,&before) && S_ISREG(before.st_mode) && before.st_size==off_t(bytes.size());
 while(okay && done<bytes.size()){
  const ssize_t n=read(fd,bytes.data()+done,bytes.size()-done);
  if(n<0 && errno==EINTR)continue;
  if(n<=0){okay=false;break;}done+=size_t(n);
 }
 okay=okay && !fstat(fd,&after) && before.st_dev==after.st_dev && before.st_ino==after.st_ino
  && before.st_size==after.st_size && before.st_mtime==after.st_mtime && before.st_ctime==after.st_ctime;
#if defined(__APPLE__)
 okay=okay && before.st_mtimespec.tv_nsec==after.st_mtimespec.tv_nsec && before.st_ctimespec.tv_nsec==after.st_ctimespec.tv_nsec;
#else
 okay=okay && before.st_mtim.tv_nsec==after.st_mtim.tv_nsec && before.st_ctim.tv_nsec==after.st_ctim.tv_nsec;
#endif
 if(close(fd))okay=false;
 return okay && done==bytes.size() && PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(bytes.data(),bytes.size()))==digest;
}
int main(int argc,char** argv){
 (void)&RmsPlanTests;assert(argc==4 || argc==5);std::vector<uint8_t> data;
 if(!ReadPinned(argv[3],20880,"75dfd208b58e224e0575e211a735f03b34b316a9e548bf63d1d48e10a261e594",data)
  || Word(data.data())!=0x31464951 || Word(data.data()+4)!=1 || Word(data.data()+8)!=248319
  || Word(data.data()+12)!=5120)return 2;
 std::array<std::array<uint32_t,5120>,3> inputs={};
 for(unsigned i=0;i<5120;i++)inputs[0][i]=Word(data.data()+16+i*4);
 const unsigned vectors=argc==5?3:1;
 if(argc==5){
  if(!ReadPinned(argv[4],290848,"85d4ada0aa8cfa847bb06e7a4296dd9992829947e8c5bf03b7ed9dadfd39f523",data)
   || Word(data.data())!=0x31464651 || Word(data.data()+4)!=1 || Word(data.data()+8)!=248319
   || Word(data.data()+12)!=5120 || Word(data.data()+16)!=17408 || Word(data.data()+20)!=72704
   || Word(data.data()+24) || Word(data.data()+28))return 2;
  for(unsigned i=0;i<5120;i++){
   inputs[1][i]=Word(data.data()+32+5120*4+i*4); // post-attention residual, input to FFN RMS
   inputs[2][i]=Word(data.data()+270368+i*4); // layer-zero final residual, next-layer input
  }
 }
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 const char* labels[]={"post-attention-normalized","post-attention-residual","layer-zero-final"};
 for(unsigned v=0;v<vectors;v++){
  const auto& input=inputs[v];LearnedRmsMachine precise(argv[1],*canonical,0,0,0),fast(argv[2],*canonical,0,0,0);
  for(unsigned i=0;i<5120;i++){
   precise.input[i]=precise.resident[i]=fast.input[i]=fast.resident[i]=input[i];
   precise.weight[i]=fast.weight[i]=PioneerAttentionBits(1.f); // isolates reduction, not the model's learned norm
  }
  assert(precise.Run() && fast.Run());precise.Verify();assert(fast.backup==input && fast.outputWrites==5120);
  double error=0,norm=0,maxAbsolute=0;unsigned changed=0;
  for(unsigned i=0;i<5120;i++){
   const double a=PioneerAttentionFloat(precise.resident[i]),b=PioneerAttentionFloat(fast.resident[i]);
   assert(std::isfinite(a) && std::isfinite(b));const double d=b-a;error+=d*d;norm+=a*a;
   maxAbsolute=std::max(maxAbsolute,std::abs(d));changed+=precise.resident[i]!=fast.resident[i];
  }
  const double relative=std::sqrt(error/(norm+1e-300));assert(relative<0.00002);
  printf("PASS emitted RV32 RMS on pinned Qwen-derived %s vector with unit weights: "
   "changed%u relative_l2%.9g max_absolute%.9g precise_instructions%u fast_instructions%u; "
   "SIMULATED ONLY, NOT learned RMS weights, full-model quality, P100A time or tokens/sec\n",
   labels[v],changed,relative,maxAbsolute,precise.steps,fast.steps);
 }
}
