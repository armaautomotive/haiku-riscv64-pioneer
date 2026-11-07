/* SPDX-License-Identifier: MIT */
// Original Q6 head bands vs CPU matrix-arithmetic model on SYNTHETIC input.
// No device, full logits, generated answer, Work, performance or quality pass.
#define PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC 1
#include "../PioneerQ38HeadBlockFloatDiagnostic.h"
#include "../PioneerBfp4PromptQuality.h"
#include "../PioneerModelWeightCache.h"
#include <cmath>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc,char** argv){
 if(argc!=2 || argv[1][0]!='/')return 2;
 int fd=open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* file=fdopen(fd,"rb");if(!file){close(fd);return 1;}
 PioneerModelWeightCache::Digest digest={};std::string error;
 puts("Authenticating original 27B GGUF before CPU-only head samples; no GPU access");fflush(stdout);
 if(!PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
  || PioneerModelWeightCache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747") {fclose(file);return 1;}
 PioneerQwen38Gguf model;if(!model.Open(file) || !model.Stable()){fclose(file);return 1;}
 const auto* head=model.Find("output.weight");if(!head){fclose(file);return 1;}
 const unsigned index=unsigned(head-model.Tensors().data());
 std::array<float,5120> input={};std::array<uint32_t,5120> bits={};
 for(unsigned i=0;i<input.size();i++){input[i]=float(int((i*73)%257)-128)*0.00390625f;std::memcpy(&bits[i],&input[i],4);}
 const unsigned bands[]={0,35456,70912,106432,141888,177344,212800,248288};
 long double energy=0,error4=0,error8=0;double max4=0,max8=0;unsigned best=0,best4=0,best8=0;double top=-INFINITY,top4=-INFINITY,top8=-INFINITY;
 for(unsigned first:bands){
  std::array<double,32> baseline={};std::array<float,5120> row={};
  for(unsigned r=0;r<32;r++){
   if(!model.ReadRow(index,first+r,row.data(),row.size()) || !model.Stable()){fclose(file);return 1;}
   for(unsigned k=0;k<5120;k++)baseline[r]+=double(row[k])*double(input[k]);
   if(!std::isfinite(baseline[r])){fclose(file);return 1;}
  }
  std::array<std::array<uint32_t,32>,2> output={};
  for(unsigned f=0;f<2;f++){
   auto format=f?PioneerBlackholeBlockFloat::Format::Bfp8B:PioneerBlackholeBlockFloat::Format::Bfp4B;
   std::vector<uint8_t> packed;
   if(!PioneerQ38HeadBlockFloatDiagnostic::PackBand(model,index,first,format,packed)
    || !PioneerBfp4PromptQuality::PredictFormat(format,packed.data(),packed.size(),bits.data(),bits.size(),output[f])){fclose(file);return 1;}
  }
  for(unsigned r=0;r<32;r++){
   float a=0,b=0;std::memcpy(&a,&output[0][r],4);std::memcpy(&b,&output[1][r],4);
   if(!std::isfinite(a) || !std::isfinite(b)){fclose(file);return 1;}
   const long double d4=static_cast<long double>(a)-baseline[r],d8=static_cast<long double>(b)-baseline[r];
   energy+=baseline[r]*baseline[r];error4+=d4*d4;error8+=d8*d8;
   max4=std::max(max4,double(std::fabs(d4)));max8=std::max(max8,double(std::fabs(d8)));
   if(baseline[r]>top){top=baseline[r];best=first+r;}if(a>top4){top4=a;best4=first+r;}if(b>top8){top8=b;best8=first+r;}
  }
  printf("completed sample band %u, synthetic input; CPU_ONLY\n",first);fflush(stdout);
 }
 if(!(energy>0) || !model.Stable()){fclose(file);return 1;}
 printf("HEAD_Q6_SAMPLE rows256 BFP4_rel_l2=%.9g BFP4_max_abs=%.9g BFP8_rel_l2=%.9g BFP8_max_abs=%.9g sampled_best_q6=%u sampled_best_bfp4=%u sampled_best_bfp8=%u\n",
  double(std::sqrt(error4/energy)),max4,double(std::sqrt(error8/energy)),max8,best,best4,best8);
 puts("PASS sampled original Q6 head, synthetic input, software arithmetic; NOT full logits, generated answer, GPU or speed");
 return fclose(file)?1:0;
}
