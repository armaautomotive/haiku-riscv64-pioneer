/* SPDX-License-Identifier: MIT */
// Local VALIDATION ONLY. Authenticated original learned weights,synthetic
// activations,full K and original per-K16 FP32 affine. NOT model generation.
#define PIONEER_Q38_EXACT_Q4_AFFINE_IMAGE_ONLY
#include "pioneer_q38_exact_q4_affine_image_test.cpp"
#include "../PioneerQ38AffineOnly.h"
#include "../PioneerQwen38Gguf.h"
#include "../PioneerModelWeightCache.h"
#include <fcntl.h>
#include <unistd.h>
#include <memory>
static bool SameSource(const struct stat& a,const struct stat& b){
 if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
 return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
struct RealAffineImage:ExactAffineImage {
 std::vector<uint8_t> original;unsigned tensor;
 RealAffineImage(const char* image,unsigned k,const std::vector<uint8_t>& rows,unsigned index,bool twoProduct=false,bool registerReplay=false):ExactAffineImage(image,k,twoProduct,registerReplay),original(rows),tensor(index){expected[0]=StageReal(3);}
 std::array<uint32_t,16> StageReal(unsigned seed){
  assert(native.Clean());using P=PioneerQ38ExactQ4Affine;using Escape=PioneerQ38Q4EscapeTiles;
  const unsigned blocks=width/256,rowBytes=blocks*144;assert(original.size()==16*rowBytes);
  std::array<float,16> expected{};
  for(unsigned block=0;block<blocks;block++){
   std::array<uint8_t,2304> tiled{};std::array<uint8_t,2816> encoded{};
   for(unsigned byte=0;byte<144;byte++)for(unsigned n=0;n<16;n++)tiled[byte*16+n]=original[n*rowBytes+block*144+byte];
   assert(Escape::Encode(tiled.data(),2304,encoded.data(),2816));
   unsigned scale=((101+(seed+tensor+block)%30)<<23)|((seed*7919+block*31)&0x7fffff);
   if((seed+block)%29==0)scale=0x80000000;else if((seed+block)&1)scale|=0x80000000;
   for(unsigned p=0;p<16;p++){
    const unsigned at=P::Source+(block*16+p)*352;for(unsigned i=0;i<352;i++)native.l1[at+i]=0;
    memcpy(native.l1.data()+at,encoded.data()+256+p*128,128);memcpy(native.l1.data()+at+128,encoded.data()+2304+p*32,32);
    int sum=0,dot[16]={};
    for(unsigned k=0;k<16;k++){
     const int x=int((seed*29+tensor*7+block*11+p*13+k*37)&255)-128;sum+=x;
     const unsigned bits=Bits(float(x));assert(!(bits&65535));native.l1[at+160+k*2]=uint8_t(bits>>16);native.l1[at+161+k*2]=uint8_t(bits>>24);
     const unsigned column=p*16+k;
     for(unsigned n=0;n<16;n++){
      const unsigned packed=original[n*rowBytes+block*144+16+(column/64)*32+column%32];
      dot[n]+=int((packed>>(column%64>=32?4:0))&15)*x;
     }
    }
    PioneerQ38Panel metadata{};assert(PioneerQ38AffineOnly(12,tiled.data(),2304,p,&metadata));
    for(unsigned n=0;n<16;n++){
     native.Write(at+192+n*4,metadata.coefficient[n]);native.Write(at+256+n*4,metadata.minimum[n]);
     // Reference independently reads ORIGINAL row-major GGUF block bytes,
     // never the source codec/panel metadata producer or FPU result.
     const uint8_t* raw=original.data()+n*rowBytes+block*144;const unsigned group=p/2;
     const unsigned factor=group<4?raw[4+group]&63:(raw[8+group]&15)|((raw[group]>>6)<<4);
     const unsigned minimum=group<4?raw[8+group]&63:(raw[8+group]>>4)|((raw[4+group]>>6)<<4);
     const float coefficient=PioneerHalfFloat(uint16_t(raw[0]|(unsigned(raw[1])<<8)))*float(factor);
     const float negativeMinimum=PioneerHalfFloat(uint16_t(raw[2]|(unsigned(raw[3])<<8)))*-float(minimum);
     assert(Bits(coefficient)==metadata.coefficient[n] && Bits(negativeMinimum)==metadata.minimum[n]);
     const float product=(coefficient*Float(scale))*float(dot[n]);const float correction=(negativeMinimum*Float(scale))*float(sum);
     expected[n]=expected[n]+(product+correction);assert(std::isfinite(expected[n]));
    }
    native.Write(at+320,scale);native.Write(at+324,unsigned(sum));
   }
  }
  native.ObserveFence();std::array<uint32_t,16> words;for(unsigned n=0;n<16;n++)words[n]=Bits(expected[n]);return words;
 }
 void NextFixture()override{if(native.allowRegisterReplay)native.replay.Poison();expected[1]=StageReal(142);}
};
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);
#ifdef PIONEER_Q38_TWO_PRODUCT_REAL_TEST
 static_assert(PIONEER_Q38_TWO_PRODUCT_REAL_TEST==1,"explicit CPU validation variant only");
 constexpr bool twoProduct=true;
 puts("TWO_PRODUCT_ACTUAL_RV32_ORIGINAL_WEIGHT_VALIDATION_ONLY_NO_DEVICE");
#else
 constexpr bool twoProduct=false;
#endif
#ifdef PIONEER_Q38_TWO_PRODUCT_REPLAY_REAL_TEST
 static_assert(PIONEER_Q38_TWO_PRODUCT_REPLAY_REAL_TEST==1 && twoProduct,"explicit replay13 two-product validation only");
 constexpr bool registerReplay=true;
 puts("REPLAY13_TWO_PRODUCT_ACTUAL_ORIGINAL_WEIGHT_VALIDATION_ONLY_NO_DEVICE");
#else
 constexpr bool registerReplay=false;
#endif
 const int fd=open(argv[4],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);assert(fd>=0);FILE* file=fdopen(fd,"rb");assert(file);
 struct stat before{},after{};assert(!fstat(fd,&before) && S_ISREG(before.st_mode));
 using Cache=PioneerModelWeightCache;Cache::Digest sha{};std::string error;
 puts("AUTH_REAL_Q4_FULLK_AFFINE_SAME_OPEN_ORIGINAL_READ_ONLY_NO_DEVICE");
 assert(Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,sha,error)
  && Cache::Hex(sha)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
 auto source=std::make_unique<PioneerQwen38Gguf>();assert(source->Open(file) && source->Tensors().size()==851);
 unsigned tensors=0,jobs=0,panelJobs=0,outputs=0,maxStack=0;
 for(unsigned index=0;index<source->Tensors().size();index++){
  const auto& t=source->Tensors()[index];if(t.type!=12)continue;
  assert(t.rank==2 && PioneerQ38ExactQ4Affine::Width(unsigned(t.inputs)) && !(t.outputs%16));
  const unsigned width=unsigned(t.inputs),rowBytes=width/256*144;
  const unsigned first=index%3==0?0:index%3==1?unsigned(t.outputs/32)*16:unsigned(t.outputs)-16;
  std::vector<uint8_t> rows(rowBytes*16);assert(source->ReadPacked(index,uint64_t(first)*rowBytes,rows.size(),rows.data(),rows.size()));
  RealAffineImage image(argv[width==5120?1:width==6144?2:3],width,rows,index,twoProduct,registerReplay);
  image.native.delay[0]=index&1?100003:1;image.native.delay[1]=index&1?1:100003;
  assert(image.Execute());maxStack=std::max(maxStack,0x70000-image.minSP);tensors++;jobs+=2;panelJobs+=2*width/16;outputs+=32;
  printf("REAL_Q4_FULLK_AFFINE tensors%u jobs%u panelJobs%u aggregateOutputs%u_ONLY\n",tensors,jobs,panelJobs,outputs);
 }
 assert(tensors==193 && jobs==386 && outputs==6176 && source->Stable());
 assert(!fstat(fd,&after) && SameSource(before,after));assert(!fclose(file));
 printf("PASS_ACTUAL_RV32_REAL_Q4_FULLK_%sAFFINE tensors%u completeProjectionJobs%u panelJobs%u exactOutputs%u stack%u;"
  "all851OriginalSourceAuthenticated;SYNTHETIC_INPUT_LOCAL_VALIDATION_NOT_MODEL_HARDWARE_OR_TPS\n",twoProduct?(registerReplay?"REPLAY13_TWO_PRODUCT_":"TWO_PRODUCT_"):"",tensors,jobs,panelJobs,outputs,maxStack);
}
