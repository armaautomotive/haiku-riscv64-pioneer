/* SPDX-License-Identifier: MIT */
// CPU-only first-position observer. Its private output is diagnostic data,
// NEVER a device input, inference fallback, Work feature or quality verdict.
#if !defined(PIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC) || PIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC != 1 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(__FAST_MATH__)
#error Exact BFP4 quality CPU diagnostic role required
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_model_reference.h"
#include "pioneer_q38_cpu_reference_math.h"
#include "../PioneerQ38PublicModelSource.h"
#include "../PioneerModelWeightCache.h"
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

class AuthenticatedRows:public Q38OriginalRows {
 PioneerQ38PublicModelSource original;
public:
 bool Open(){return original.Open() && original.Ready();}
 bool Ready()const override{return original.Ready();}
 uint64_t Epoch()const override{return Ready()?original.Model().Epoch():0;}
 bool Describe(unsigned index,Q38OriginalTensor& out)const override{
  if(!Ready() || index>=original.Model().Tensors().size())return false;
  const auto& t=original.Model().Tensors()[index];
  Q38OriginalTensor next{t.name,t.type,t.rank,t.inputs,t.outputs,t.bytes,t.offset};
  if(!Ready())return false;
  out=std::move(next);return true;
 }
 bool ReadOriginal(unsigned tensor,uint64_t at,unsigned bytes,void* out,unsigned capacity)override{
  return original.ReadReferencePacked(tensor,at,bytes,out,capacity) && Ready();
 }
};
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--first-prompt-input")){fputs("Use --first-prompt-input; CPU-only.\n",stderr);return 2;}
 const char* directory="/boot/home/pioneer-bfp4-prompt-quality-20261106-a";
 const char* output="/boot/home/pioneer-bfp4-prompt-quality-20261106-a/prompt-input.bin";
 struct stat info={};
 if(lstat(directory,&info) || !S_ISDIR(info.st_mode) || info.st_uid!=geteuid() || (info.st_mode&07777)!=0700)return 1;
 const uint32_t endian=1;if(*reinterpret_cast<const uint8_t*>(&endian)!=1)return 1;
 puts("PROMPT_INPUT: authenticating original model; CPU diagnostic only, no device");if(fflush(stdout))return 1;
 AuthenticatedRows source;if(!source.Open())return 1;
 Q38CpuModelReference observer;if(!observer.Bind(source))return 1;
 std::vector<uint32_t> input,final;
 if(!observer.ObserveFirstFfnInput(248319,input,final) || observer.Ready() || !source.Ready() || input.size()!=5120 || final.size()!=5120)return 1;
 const auto finalHash=PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(final.data(),final.size()*sizeof(uint32_t)));
 if(finalHash!="0fc71cc5a79d621c65b45aceb6bf705b1989d2b2385af99c8e2a2fbb5e0c064c"){
  fputs("PROMPT_INPUT_REFUSED: first-layer output differs from pinned CPU trace; no input published\n",stderr);return 1;
 }
 Q38OriginalTensor tensor;unsigned index=851;
 for(unsigned i=0;i<851;i++){Q38OriginalTensor candidate;if(!source.Describe(i,candidate))return 1;
  if(candidate.name=="blk.0.ffn_gate.weight"){if(index!=851)return 1;index=i;tensor=std::move(candidate);}}
 if(index>=851 || tensor.type!=12 || tensor.inputs!=5120 || tensor.outputs!=17408)return 1;
 std::vector<float> realInput;realInput.reserve(5120);for(auto word:input)realInput.push_back(Q38CpuReferenceMath::Float(word));
 std::vector<uint32_t> data={0x31464951,1,248319,5120};data.insert(data.end(),input.begin(),input.end());
 for(unsigned first:{0U,8704U,17376U}){
  std::vector<float> baseline;
  auto read=[&](uint64_t at,unsigned bytes,void* out){return source.ReadOriginal(index,uint64_t(first)*2880+at,bytes,out,bytes);};
  if(!Q38CpuReferenceMath::Matrix(12,5120,32,realInput,baseline,read) || baseline.size()!=32 || !source.Ready())return 1;
  for(float value:baseline)data.push_back(Q38CpuReferenceMath::Bits(value));
 }
 const auto digest=PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(data.data(),data.size()*sizeof(uint32_t)));
 if(!source.Ready())return 1;
 int fd=open(output,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);if(fd<0)return 1;
 const uint8_t* bytes=reinterpret_cast<const uint8_t*>(data.data());size_t done=0,total=data.size()*sizeof(uint32_t);
 while(done<total){ssize_t n=write(fd,bytes+done,total-done);if(n<=0){close(fd);return 1;}done+=size_t(n);}
 const bool synced=!fsync(fd),closed=!close(fd);
 if(!synced || !closed || !source.Ready())return 1;
 printf("PASS_PROMPT_INPUT token248319 layer0 input_words5120 original_q4_words96 pinned_final_hidden_exact1 sha256%s; CPU_ONLY_NO_DEVICE_NO_QUALITY_VERDICT\n",digest.c_str());
 return fflush(stdout)?1:0;
}
