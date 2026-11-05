/* SPDX-License-Identifier: MIT */
// OFFBOARD real original integer weights + SYNTHETIC quantized activations.
// No full projection/model inference,device,cache or native authority.
#define PIONEER_Q38_EXACT_Q4_CORRECTED_IMAGE_ONLY
#include "pioneer_q38_exact_q4_corrected_image_test.cpp"
#include "../PioneerQwen38Gguf.h"
#include <fcntl.h>
#include <unistd.h>
#include <memory>
static bool SameSource(const struct stat& a,const struct stat& b) {
 if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
 return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
int main(int argc,char** argv) {
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 int fd=open(argv[2],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);
 FILE* file=fdopen(fd,"rb");assert(file);struct stat before{},after{};assert(!fstat(fd,&before) && S_ISREG(before.st_mode));
 using Cache=PioneerModelWeightCache;Cache::Digest sha{};std::string error;
 puts("AUTH_REAL_Q4_SAME_OPEN_ORIGINAL_READ_ONLY_NO_DEVICE");
 assert(Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,sha,error)
  && Cache::Hex(sha)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
 auto source=std::make_unique<PioneerQwen38Gguf>();assert(source->Open(file) && source->Tensors().size()==851);
 unsigned tensors=0,tiles=0,panels=0,dots=0;
 for(unsigned index=0;index<source->Tensors().size();index++) {
  const auto& t=source->Tensors()[index];if(t.type!=12)continue;
  assert(t.rank==2 && !(t.inputs%256) && !(t.outputs%16));const unsigned blocks=unsigned(t.inputs/256),rowBytes=blocks*144;
  const uint64_t tileCount=t.outputs/16*blocks;
  for(uint64_t tile:{UINT64_C(0),tileCount/2,tileCount-1}) {
   const unsigned first=unsigned(tile/blocks)*16,block=unsigned(tile%blocks);
   std::vector<uint8_t> rows(rowBytes*16);assert(source->ReadPacked(index,uint64_t(first)*rowBytes,rows.size(),rows.data(),rows.size()));
   std::array<uint8_t,2304> tiled{};std::array<uint8_t,2816> encoded{};
   for(unsigned byte=0;byte<144;byte++)for(unsigned n=0;n<16;n++)tiled[byte*16+n]=rows[n*rowBytes+block*144+byte];
   assert(PioneerQ38Q4EscapeTiles::Encode(tiled.data(),2304,encoded.data(),2816));
   for(unsigned p=0;p<16;p++) {
    CorrectedImage image(argv[1]);std::array<int,16> input{};std::array<float,16> expected{};
    for(unsigned k=0;k<16;k++) {
     input[k]=int((index*7+unsigned(tile)*13+p*29+k*17)&255)-128;
     const unsigned column=p*16+k;
     for(unsigned n=0;n<16;n++) {
      const unsigned packed=rows[n*rowBytes+block*144+16+(column/64)*32+column%32];
      const unsigned q=(packed>>(column%64>=32?4:0))&15;expected[n]+=float(int(q)*input[k]);
     }
    }
    image.native.StageCorrected(encoded.data()+256+p*128,encoded.data()+2304+p*32,input);
    image.native.delay[0]=p&1?100003:1;image.native.delay[1]=p&1?1:100003;
    assert(image.Execute());image.Exact(expected);panels++;dots+=32;
   }
   tiles++;
  }
  tensors++;printf("REAL_Q4_CORRECTED_TENSOR%u_TILES%u_PANELS%u_AGGREGATES_ONLY\n",tensors,tiles,panels);
 }
 assert(tensors==193 && tiles==579 && panels==9264 && dots==296448 && source->Stable());
 assert(!fstat(fd,&after) && SameSource(before,after));assert(!fclose(file));
 printf("PASS_ACTUAL_IMAGE_REAL_Q4 tensors%u firstMiddleLastTiles%u panels%u originalIntegerDots%u all851SourceAuthenticated;"
  "SYNTHETIC_INPUT_NOT_FULLK_AFFINE_MODEL_QUALITY_DEVICE_OR_TPS\n",tensors,tiles,panels,dots);
}
