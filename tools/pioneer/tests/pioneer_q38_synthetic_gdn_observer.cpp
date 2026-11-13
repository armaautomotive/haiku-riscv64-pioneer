/* SPDX-License-Identifier: MIT */
// Separate host-only reference observer. No device or Work-mode code.
#if !defined(PIONEER_Q38_SYNTHETIC_GDN_OBSERVER_TEST) || PIONEER_Q38_SYNTHETIC_GDN_OBSERVER_TEST != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Synthetic GDN observer requires its isolated host role
#endif
#define PIONEER_Q38_GDN_RENDEZVOUS_VALIDATION_CLIENT 1
#define PIONEER_Q38_CPU_VALIDATION 1
#define PIONEER_Q38_LEARNED_RMS_VALIDATION_REFERENCE 1
#include "../PioneerQ38GdnRendezvousValidationReference.h"
namespace {
struct Source {
 struct Tensor{std::string name;unsigned type=0,rank=1,inputs=48,outputs=1,bytes=192;};
 struct ModelType {
  std::vector<Tensor> tensors={{"blk.0.ssm_a"},{"blk.0.ssm_dt.bias"},{"blk.0.ssm_norm.weight",0,1,128,1,512}};
  const Tensor* Find(const char* name)const{for(const auto& t:tensors)if(t.name==name)return &t;return nullptr;}
  const auto& Tensors()const{return tensors;}
 } model;
 bool Ready()const{return true;}const auto& Model()const{return model;}
 bool ReadReferencePacked(unsigned tensor,uint64_t offset,unsigned bytes,void* out,unsigned capacity){
  if(tensor>=3 || offset || bytes!=(tensor==2?512U:192U) || capacity!=bytes || !out)return false;
  const uint32_t bits=tensor==0?0xbf800000U:tensor==2?0x3f800000U:0U;
  for(unsigned i=0;i<bytes/4;i++)for(unsigned b=0;b<4;b++)static_cast<uint8_t*>(out)[i*4+b]=uint8_t(bits>>(8*b));
  return true;
 }
};
}
extern "C" bool Q38SyntheticGdnExpected(const uint32_t* conv,const uint32_t* alpha,const uint32_t* beta,uint32_t* output){
 Source source;return PioneerQ38GdnRecurrentReference(source,conv,40960,alpha,beta,48,output,823488);
}
extern "C" bool Q38SyntheticGdnGatedExpected(const uint32_t* conv,const uint32_t* alpha,const uint32_t* beta,const uint32_t* gate,uint32_t* output){
 Source source;return PioneerQ38GdnRendezvousReference(source,conv,40960,alpha,beta,48,gate,6144,output,829632);
}
