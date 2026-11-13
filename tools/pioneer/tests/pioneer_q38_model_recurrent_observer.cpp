/* SPDX-License-Identifier: MIT */
// Host-only numerical observer for pinned, model-derived BFP8 exports.
#if !defined(PIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST) || PIONEER_Q38_MODEL_RECURRENT_OBSERVER_TEST != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Model recurrent observer is an isolated host test
#endif
#include "../PioneerBlackholeBlockFloat.h"
#include <array>
#include <cstdio>
#include <cstdint>

extern "C" bool Q38ModelRecurrentExpected(const char* path,const float* input,double* output){
 if(!path || !input || !output)return false;
 FILE* file=std::fopen(path,"rb");if(!file)return false;
 uint32_t h[8];bool ok=std::fread(h,4,8,file)==8 && h[0]==0x31525051 && h[1]==1
  && h[3]==5120 && h[4]==64 && h[5]==2 && h[6]==160 && h[7]==1088;
 std::array<uint8_t,1088> packed;std::array<float,1024> tile;
 for(unsigned row=0;row<64;row++)output[row]=0;
 for(unsigned band=0;band<2 && ok;band++){
  for(unsigned t=0;t<160 && ok;t++){
   ok=std::fread(packed.data(),1,packed.size(),file)==packed.size()
    && PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp8B,packed.data(),packed.size(),tile.data(),tile.size());
   if(ok)for(unsigned row=0;row<32;row++)for(unsigned col=0;col<32;col++)
    output[band*32+row]+=double(tile[row*32+col])*double(input[t*32+col]);
  }
  uint32_t exportedReference[32];if(ok)ok=std::fread(exportedReference,4,32,file)==32;
 }
 if(ok)ok=std::fgetc(file)==EOF && !std::ferror(file);
 if(std::fclose(file))ok=false;return ok;
}
