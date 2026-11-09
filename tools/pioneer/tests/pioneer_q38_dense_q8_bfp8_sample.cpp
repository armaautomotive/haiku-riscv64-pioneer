/* SPDX-License-Identifier: MIT */
// Authenticated real-model CPU fidelity sample; not device arithmetic or Work.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(__FAST_MATH__)
#error Offline numerical sample only
#endif
#define PIONEER_Q38_FULL_MODEL_CPU_REFERENCE 1
#define PIONEER_Q38_CPU_VALIDATION 1
#include "pioneer_q38_cpu_reference_math.h"
#include "pioneer_q38_whole_original_bytes.h"
#include "../PioneerQ38NativeWeightTiles.h"
#include "../PioneerModelWeightCache.h"
#include <cstdio>
#include <map>
#include <memory>
#include <unistd.h>
using namespace Q38CpuReferenceMath;
#include "actual_cpu_replay_impl.inc"
static bool ReadHidden(const char* path,std::vector<float>& output){
 FILE* f=std::fopen(path,"rb");if(!f)return false;std::vector<float> stage(5120);
 bool ok=!std::fseek(f,2*5120*4,SEEK_SET) && std::fread(stage.data(),4,stage.size(),f)==stage.size();
 if(std::fclose(f))ok=false;for(float x:stage)if(!std::isfinite(x))ok=false;
 if(ok)output=std::move(stage);return ok;
}
static bool Bf16(float input,float& output){
 unsigned bits=Bits(input),exp=bits&0x7f800000;
 if(exp==0x7f800000 || (!exp && (bits&0x7fffff)))return false;
 unsigned rounded=(bits+0x7fff+((bits>>16)&1))>>16;
 if((rounded&0x7f80)==0x7f80 || (!(rounded&0x7f80) && (rounded&127)))return false;
 output=Float(rounded<<16);return true;
}
int main(int argc,char** argv){
 if(argc!=3 || std::fegetround()!=FE_TONEAREST)return 2;
 float unit=0,minus=0,bad=0;
 if(!Bf16(1.f,unit) || !Bf16(-2.f,minus) || unit!=1.f || minus!=-2.f
  || Bf16(Float(0x7fc00000),bad) || Bf16(Float(1),bad))return 1;
 std::vector<float> hidden,weight,input;
 if(!ReadHidden(argv[2],hidden))return 1;
 WholeModelOriginalBytes original;if(!original.Open(argv[1]) || !original.Ready())return 1;
 Q38LayerReplay replay;if(!replay.Bind(original) || !replay.Weight(3,0,weight) || !Rms(hidden,weight,input))return 1;
 std::unique_ptr<FILE,decltype(&std::fclose)> file(std::fopen(argv[1],"rb"),&std::fclose);
 PioneerQwen38Gguf source;if(!file || !source.Open(file.get()))return 1;
 PioneerModelWeightCache::Digest hash{};std::string error;
 if(!PioneerModelWeightCache::HashFile(fileno(file.get()),PioneerQwen38Gguf::FullBytes,hash,error)
  || PioneerModelWeightCache::Hex(hash)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747" || !source.Stable())return 1;
 const char* names[]={"blk.3.attn_q.weight","blk.3.attn_k.weight","blk.3.attn_v.weight"};
 using Tiles=PioneerQ38NativeWeightTiles;using Codec=PioneerBlackholeBlockFloat;
 for(unsigned kind=0;kind<3;kind++){
  unsigned index=source.Tensors().size();for(unsigned i=0;i<source.Tensors().size();i++)if(source.Tensors()[i].name==names[kind])index=i;
  if(index>=source.Tensors().size() || source.Tensors()[index].type!=8 || source.Tensors()[index].inputs!=5120)return 1;
  std::vector<float> reference;if(!replay.Project(3,5+kind,input,reference) || reference.size()!=source.Tensors()[index].outputs)return 1;
  const unsigned bands=reference.size()/32;
  for(unsigned band: {0U,bands/2,bands-1}){
   double sums[32]={},weightError=0,weightEnergy=0;
   for(unsigned tile=0;tile<160;tile++){
    std::array<float,1024> originalTile{},decoded{};std::array<uint8_t,1088> packed{};
    if(!Tiles::ReadFloatTile(source,index,uint64_t(band)*160+tile,originalTile.data(),1024)
     || !Codec::PackTile(Codec::Format::Bfp8B,originalTile.data(),1024,packed.data(),packed.size())
     || !Codec::UnpackTile(Codec::Format::Bfp8B,packed.data(),packed.size(),decoded.data(),1024))return 1;
    for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++){
     const unsigned i=row*32+col;float in=0;if(!Bf16(input[tile*32+col],in))return 1;
     sums[row]+=double(decoded[i])*in;const double d=double(decoded[i])-originalTile[i];
     weightError+=d*d;weightEnergy+=double(originalTile[i])*originalTile[i];
    }
   }
   double err=0,norm=0,dot=0,actual=0;
   for(unsigned row=0;row<32;row++){
    const double ref=reference[band*32+row],d=sums[row]-ref;err+=d*d;norm+=ref*ref;dot+=sums[row]*ref;actual+=sums[row]*sums[row];
   }
   if(!(norm>0) || !(actual>0) || !(weightEnergy>0))return 1;
   std::printf("DENSE_Q8_BFP8_SAMPLE tensor=%s first_row=%u rows=32 weight_rel_l2=%.9g output_rel_l2=%.9g cosine=%.9g; CPU_ONLY\n",
    names[kind],band*32,std::sqrt(weightError/weightEnergy),std::sqrt(err/norm),dot/std::sqrt(norm*actual));std::fflush(stdout);
  }
 }
 if(!source.Stable() || !original.Ready())return 1;
 std::puts("NINE_BANDS_ONLY; REAL MODEL WEIGHTS + PINNED INPUT, IDEAL_CPU_DOT NOT DEVICE OR TPS");
}
