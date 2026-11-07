/* SPDX-License-Identifier: MIT */
// CPU-only first-layer BFP4 approximation; no accelerator use or fallback.
#define main PioneerPriorPromptQualityMain
#include "pioneer_bfp4_prompt_quality.cpp"
#undef main
#include "PioneerBfp4FfnQuality.h"
#include "PioneerQwen38Gguf.h"
#include <memory>

static const char* stageName="";
static bool Progress(unsigned row,unsigned total){
 printf("BFP4_FFN_PROGRESS stage%s rows%u/%u; CPU_ONLY\n",stageName,row,total);return !fflush(stdout);
}
static bool Report(const char* name,const std::vector<uint32_t>& got,const std::vector<uint32_t>& expected){
 PioneerBfp4PromptQuality::Result result={};if(!PioneerBfp4FfnQuality::Compare(got,expected,result))return false;
 printf("BFP4_FFN_COMPARE stage%s values%zu relative_l2%.9g max_abs%.9g; CPU_DIAGNOSTIC_NOT_FULL_MODEL_QUALITY_OR_SPEED\n",
  name,got.size(),result.relativeL2,result.maxAbsolute);return !fflush(stdout);
}
int main(int argc,char** argv){
 if(argc!=2 || strlen(argv[1])!=64 || strspn(argv[1],"0123456789abcdef")!=64)return 2;
 std::vector<uint8_t> raw,prior;
 if(!ReadPinned("/boot/home/pioneer-bfp4-ffn-quality-20261106-a/reference.bin",72712*4,argv[1],raw)
  || !ReadPinned("/boot/home/pioneer-bfp4-prompt-quality-20261106-a/prompt-input.bin",16+(5120+96)*4,
   "75dfd208b58e224e0575e211a735f03b34b316a9e548bf63d1d48e10a261e594",prior))return 1;
 const unsigned expected[]={0x31464651,1,248319,5120,17408,72704,0,0};
 for(unsigned i=0;i<8;i++)if(Word(raw.data()+4*i)!=expected[i])return 1;
 if(Word(prior.data())!=0x31464951 || Word(prior.data()+4)!=1 || Word(prior.data()+8)!=248319 || Word(prior.data()+12)!=5120)return 1;
 unsigned at=8;auto take=[&](unsigned n){std::vector<uint32_t> v(n);for(unsigned i=0;i<n;i++)v[i]=Word(raw.data()+4*(at++));return v;};
 auto input=take(5120),residual=take(5120),gateRef=take(17408),upRef=take(17408),activationRef=take(17408),downRef=take(5120),finalRef=take(5120);
 if(at!=72712)return 1;
 for(unsigned i=0;i<5120;i++)if(input[i]!=Word(prior.data()+16+i*4))return 1;
 const unsigned firsts[]={0,8704,17376};
 for(unsigned sample=0;sample<3;sample++)for(unsigned row=0;row<32;row++)
  if(gateRef[firsts[sample]+row]!=Word(prior.data()+16+(5120+sample*32+row)*4))return 1;
 const char* path="/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf";
 int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || info.st_uid!=geteuid() || info.st_nlink!=1
  || (info.st_mode&0022) || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* stream=fdopen(fd,"rb");if(!stream){close(fd);return 1;}
 std::unique_ptr<FILE,decltype(&fclose)> file(stream,&fclose);
 puts("BFP4_FFN_AUTHENTICATING_ORIGINAL; CPU_ONLY_NO_DEVICE");if(fflush(stdout))return 1;
 PioneerModelWeightCache::Digest digest={};std::string error;
 if(!PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
  || PioneerModelWeightCache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747")return 1;
 PioneerQwen38Gguf model;struct stat after={};
 if(!model.Open(file.get()) || !model.Stable() || fstat(fd,&after) || !Same(info,after))return 1;
 const char* names[]={"blk.0.ffn_gate.weight","blk.0.ffn_up.weight","blk.0.ffn_down.weight"};
 unsigned indices[3]={};
 for(unsigned i=0;i<3;i++){
  const auto* tensor=model.Find(names[i]);if(!tensor || tensor->type!=12 || tensor->rank!=2
   || tensor->inputs!=(i==2?17408U:5120U) || tensor->outputs!=(i==2?5120U:17408U))return 1;
  indices[i]=unsigned(tensor-model.Tensors().data());
 }
 const char* samplePins[]={"edbc6d7cadcd85d0114ff16a5c44afb3627296497a7a0e03f76efa41da5ac7dd",
  "59f05959ef8486021eefd0b7e02e083b6f18a7eef747f15d5e24e6e87c587695",
  "036bcee72fc9bb84fedb9c6f93a6d22358f9d7c90c610cc5c19159f002a587e3"};
 for(unsigned sample=0;sample<3;sample++){
  char name[160];snprintf(name,sizeof(name),"/boot/home/pioneer-bfp4-real-band-export-20261106-a/private/band%u.rbp4",sample);
  std::vector<uint8_t> old,packed;
  if(!ReadPinned(name,102544,samplePins[sample],old) || Word(old.data())!=0x34504252 || Word(old.data()+12)!=firsts[sample]
   || !PioneerBfp4FfnQuality::PackBand(model,indices[0],firsts[sample],packed) || packed.size()!=160*576
   || memcmp(packed.data(),old.data()+16,packed.size()))return 1;
 }
 puts("BFP4_FFN_PACKING_MATCHES_THREE_PRIOR_CARD_BANDS; CPU_ONLY");if(fflush(stdout))return 1;
 std::vector<uint32_t> gate,up,activation,down,final;
 stageName="gate";if(!PioneerBfp4FfnQuality::Project(model,indices[0],input,gate,Progress) || !Report("gate",gate,gateRef))return 1;
 stageName="up";if(!PioneerBfp4FfnQuality::Project(model,indices[1],input,up,Progress) || !Report("up",up,upRef))return 1;
 if(!PioneerBfp4FfnQuality::Activate(gate,up,activation) || !Report("activation",activation,activationRef))return 1;
 stageName="down";if(!PioneerBfp4FfnQuality::Project(model,indices[2],activation,down,Progress) || !Report("down",down,downRef))return 1;
 final.resize(5120);for(unsigned i=0;i<5120;i++){
  float value=PioneerBfp4FfnQuality::Float(residual[i])+PioneerBfp4FfnQuality::Float(down[i]);
  if(!std::isfinite(value))return 1;
  final[i]=PioneerBfp4FfnQuality::Bits(value);
 }
 if(!Report("final_hidden",final,finalRef) || !model.Stable())return 1;
 puts("PASS_BFP4_FIRST_FFN_COMPARE token248319 layer0 all_gate_up_down; CPU_ONLY_NO_DEVICE_NO_FULL_MODEL_QUALITY_OR_SPEED");
 return fflush(stdout)?1:0;
}
