/* SPDX-License-Identifier: MIT */
// Offline reference replay, not Work mode or a device execution.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(__FAST_MATH__)
#error Offline reference replay only
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include "pioneer_q38_whole_original_bytes.h"
#include <cstdio>
#include <map>
using namespace Q38CpuReferenceMath;
#include "actual_cpu_replay_impl.inc"
static bool Read(const char* path,long offset,unsigned n,void* dst){
 FILE* f=std::fopen(path,"rb");if(!f)return false;
 bool okay=!std::fseek(f,offset,SEEK_SET) && std::fread(dst,1,n,f)==n;
 if(std::fclose(f))okay=false;return okay;
}
int main(int argc,char** argv){
 static_assert(sizeof(float)==4 && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"little-endian float required");
 if(argc!=5 || std::fegetround()!=FE_TONEAREST)return 2;
 if(HalfRne(1.f)!=0x3c00 || HalfRne(-2.f)!=0xc000 || HalfRne(0.f)!=0)return 1;
 std::vector<float> hidden(5120),reference(6144),raw(1024),weights,normalized,qg,value;
 std::vector<uint16_t> packed(1024);
 if(!Read(argv[2],2*5120*4,5120*4,hidden.data()) || !Read(argv[3],0,4096,raw.data())
  || !Read(argv[3],4096,2048,packed.data()) || !Read(argv[4],0,6144*4,reference.data()))return 1;
 for(float f:hidden)if(!std::isfinite(f))return 1;
 for(float f:raw)if(!std::isfinite(f))return 1;
 for(float f:reference)if(!std::isfinite(f))return 1;
 WholeModelOriginalBytes model;if(!model.Open(argv[1]) || !model.Ready())return 1;
 Q38LayerReplay replay;if(!replay.Bind(model) || !replay.Weight(3,0,weights) || !Rms(hidden,weights,normalized)
  || !replay.Project(3,5,normalized,qg) || !replay.Project(3,7,normalized,value)
  || qg.size()!=12288 || value.size()!=1024)return 1;
 for(unsigned h=0;h<24;h++)for(unsigned i=0;i<256;i++){
  float expected=Flush(Half(HalfRne(value[(h/6)*256+i]))*Sigmoid(qg[h*512+256+i]));
  if(Bits(expected)!=Bits(reference[h*256+i])){
   std::fprintf(stderr,"CPU_REFERENCE_MISMATCH head=%u lane=%u expected=%08x pinned=%08x\n",
    h,i,Bits(expected),Bits(reference[h*256+i]));return 1;
  }
 }
 for(unsigned group=0;group<4;group++){
  unsigned bits=0,signs=0,packedMismatch=0;double err=0,den=0,dot=0,observed=0;
  for(unsigned i=group*256;i<(group+1)*256;i++){
   const float a=raw[i],b=value[i];const double delta=double(a)-b;
   bits+=Bits(a)!=Bits(b);signs+=double(a)*b<0;packedMismatch+=packed[i]!=HalfRne(a);
   err+=delta*delta;den+=double(b)*b;dot+=double(a)*b;observed+=double(a)*a;
  }
  if(!(den>0) || !(observed>0) || !std::isfinite(err) || !std::isfinite(dot))return 1;
  std::printf("VALUE_CAPTURE group=%u raw_bits_changed=%u opposite_signs=%u rel_l2=%.12g cosine=%.12g packed_vs_raw_mismatch=%u\n",
   group,bits,signs,std::sqrt(err/den),dot/std::sqrt(den*observed),packedMismatch);
 }
 if(!model.Ready())return 1;
 std::puts("CPU_ONLY DATA SCREEN; source identity and completion not established; NO DEVICE OR SPEED");
}
