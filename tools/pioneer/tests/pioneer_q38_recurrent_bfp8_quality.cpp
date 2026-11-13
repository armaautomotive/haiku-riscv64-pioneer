/* SPDX-License-Identifier: MIT */
// Offline sampled conversion error versus authenticated original Q8 values.
// Not a model-quality, hardware-arithmetic, or inference-speed test.
#include "../PioneerQ38NativeBfpBandSource.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unistd.h>

int main(int argc,char** argv) {
 if(argc!=2 || argv[1][0]!='/')return 2;
 std::unique_ptr<FILE,decltype(&fclose)> file(fopen(argv[1],"rb"),&fclose);
 if(!file)return 1;
 PioneerQwen38Gguf model; PioneerModelWeightCache::Digest digest{};std::string error;
 if(!model.Open(file.get()) || !PioneerModelWeightCache::HashFile(fileno(file.get()),
  PioneerQwen38Gguf::FullBytes,digest,error) || digest!=PioneerQ38NativeBfpBandSource::OriginalPin()
  || !model.Stable())return 1;
 const char* names[]={"attn_qkv.weight","attn_gate.weight","ssm_out.weight"};
 for(const char* name:names) {
  long double squares=0,errorSquares=0;double maxError=0;unsigned tiles=0;uint64_t values=0;
  for(unsigned layer:{0U,16U,32U}) {
   const std::string key="blk."+std::to_string(layer)+"."+name;
   const auto* tensor=model.Find(key.c_str());
   if(!tensor || tensor->type!=8 || tensor->inputs%32 || tensor->outputs%32)return 1;
   unsigned index=unsigned(tensor-model.Tensors().data());
   uint64_t rows=tensor->outputs/32,cols=tensor->inputs/32;
   for(uint64_t row:{UINT64_C(0),rows/2,rows-1})for(uint64_t col:{UINT64_C(0),cols/2,cols-1}) {
    std::array<float,1024> original{},decoded{};std::array<uint8_t,1088> packed{};
    using Codec=PioneerBlackholeBlockFloat;
    if(!PioneerQ38NativeWeightTiles::ReadFloatTile(model,index,row*cols+col,original.data(),1024)
     || !Codec::PackTile(Codec::Format::Bfp8B,original.data(),1024,packed.data(),packed.size())
     || !Codec::UnpackTile(Codec::Format::Bfp8B,packed.data(),packed.size(),decoded.data(),1024))return 1;
    for(unsigned i=0;i<1024;i++) {
     if(!std::isfinite(original[i]) || !std::isfinite(decoded[i]))return 1;
     double e=double(decoded[i])-original[i];errorSquares+=e*e;squares+=double(original[i])*original[i];
     maxError=std::max(maxError,std::abs(e));values++;
    }
    tiles++;
   }
  }
  if(!squares || !model.Stable())return 1;
  std::printf("%s tiles=%u values=%llu relative_rms=%.9g max_abs=%.9g\n",name,tiles,
   (unsigned long long)values,std::sqrt(double(errorSquares/squares)),maxError);
 }
 puts("SAMPLED WEIGHT ERROR ONLY; NO DEVICE, MODEL QUALITY OR SPEED");
 return 0;
}
