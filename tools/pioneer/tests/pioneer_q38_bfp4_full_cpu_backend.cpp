/* SPDX-License-Identifier: MIT */
// Separate TU: intentionally cannot include the CPU reference's role macros.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error BFP4 full-model backend is CPU-only diagnostic code
#endif
#include "pioneer_q38_bfp4_full_cpu_backend.h"
#include "../PioneerBfp4FfnQuality.h"
#include "../PioneerQwen38Gguf.h"
#include "../PioneerModelWeightCache.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <memory>

namespace {
struct Context {
 FILE* file=nullptr;PioneerQwen38Gguf model;unsigned next=0;uint64_t layers=0,bfp8Layers=0;
 ~Context(){if(file)fclose(file);}
 void Skip(){while(next<192 && !(layers & (uint64_t(1)<<(next/3))))next+=3;}
};
}
void* Q38Bfp4CpuOpen(const char* path,uint64_t layers,uint64_t bfp8Layers){
 if(!path || path[0]!='/' || !layers || (bfp8Layers & ~layers))return nullptr;
 int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return nullptr;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return nullptr;}
 auto context=std::unique_ptr<Context>(new Context);
 context->file=fdopen(fd,"rb");if(!context->file){close(fd);return nullptr;}
 PioneerModelWeightCache::Digest digest={};std::string error;
 if(!PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
  || PioneerModelWeightCache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"
  || !context->model.Open(context->file) || !context->model.Stable())return nullptr;
 context->layers=layers;context->bfp8Layers=bfp8Layers;context->Skip();return context.release();
}
void Q38Bfp4CpuClose(void* context){delete static_cast<Context*>(context);}
bool Q38Bfp4CpuComplete(void* context){
 auto* current=static_cast<Context*>(context);return current && current->next==192 && current->model.Stable();
}
bool Q38Bfp4CpuProject(void* opaque,unsigned layer,unsigned role,const std::vector<float>& input,std::vector<float>& output){
 if(!opaque || layer>=64 || role<2 || role>4)return false;
 auto& context=*static_cast<Context*>(opaque);
 if(context.next!=layer*3+role-2 || !context.model.Stable())return false;
 const char* names[]={"ffn_gate.weight","ffn_up.weight","ffn_down.weight"};
 std::string name="blk."+std::to_string(layer)+"."+names[role-2];
 const auto* tensor=context.model.Find(name.c_str());
 if(!tensor || tensor->type!=12 || tensor->rank!=2 || tensor->inputs!=input.size()
  || tensor->inputs!=(role==4?17408U:5120U) || tensor->outputs!=(role==4?5120U:17408U))return false;
 std::vector<uint32_t> bits;bits.reserve(input.size());
 for(float value:input){if(!std::isfinite(value))return false;bits.push_back(PioneerBfp4FfnQuality::Bits(value));}
 std::vector<uint32_t> result;
 const unsigned index=unsigned(tensor-context.model.Tensors().data());
 const bool use8=context.bfp8Layers & (uint64_t(1)<<layer);
 const auto format=use8?PioneerBlackholeBlockFloat::Format::Bfp8B:PioneerBlackholeBlockFloat::Format::Bfp4B;
 printf("BFP%u_CPU_BEGIN layer%u role%u; CPU_ONLY\n",use8?8U:4U,layer,role);if(fflush(stdout))return false;
 if(!PioneerBfp4FfnQuality::ProjectFormat(context.model,index,bits,format,result) || !context.model.Stable() || result.size()!=tensor->outputs)return false;
 std::vector<float> values;values.reserve(result.size());
 for(uint32_t word:result){float value=PioneerBfp4FfnQuality::Float(word);if(!std::isfinite(value))return false;values.push_back(value);}
 output=std::move(values);context.next++;if(context.next%3==0)context.Skip();return true;
}
