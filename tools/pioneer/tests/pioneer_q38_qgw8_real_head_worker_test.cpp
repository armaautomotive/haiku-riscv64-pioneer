/* SPDX-License-Identifier: MIT */
// Real authenticated output-head bands through the actual emitted QGW8 RV32
// worker in a simulated fabric/FPU. Injected preceding layer history and
// synthetic BF16 input: not hardware, full inference, quality or speed.
#define PIONEER_Q38_HYBRID_WORKER_MACHINE_ONLY
#define PIONEER_TEST_BFP8 1
#define PIONEER_Q38_HEAD_BLOCK_FLOAT_DIAGNOSTIC 1
#include "pioneer_q38_hybrid_worker_image_test.cpp"
#include "../PioneerQ38HeadBlockFloatDiagnostic.h"
#include "../PioneerBfp4PromptQuality.h"
#include "../PioneerModelWeightCache.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static FILE* NewPrivateFile(const std::string& path){
 int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
 if(fd<0)return nullptr;FILE* f=fdopen(fd,"wb");if(!f)close(fd);return f;
}
static bool WriteWords(FILE* f,const char* name,const std::vector<uint8_t>& bytes){
 if(bytes.size()%4)return false;
 if(fprintf(f," static const uint32_t %s[%zu]={\n",name,bytes.size()/4)<0)return false;
 for(size_t i=0;i<bytes.size();i+=4){
  uint32_t word=uint32_t(bytes[i])|uint32_t(bytes[i+1])<<8|uint32_t(bytes[i+2])<<16|uint32_t(bytes[i+3])<<24;
  if(fprintf(f,"%s0x%08x%s",i%32?"":"  ",word,i+4==bytes.size()?"\n":i%32==28?",\n":",")<0)return false;
 }
 return fputs(" };\n",f)>=0;
}
static bool FinishPrivate(FILE* f){bool good=!ferror(f) && !fflush(f) && !fsync(fileno(f));return !fclose(f) && good;}

int main(int argc,char** argv){
 if((argc<4 || argc>6) || argv[1][0]!='/' || argv[2][0]!='/' || argv[3][0]!='/' || (argc>=5 && argv[4][0]!='/') || (argc==6 && argv[5][0]!='/'))return 2;
 if(argc==6){struct stat d={};if(lstat(argv[5],&d) || !S_ISDIR(d.st_mode) || d.st_uid!=geteuid() || (d.st_mode&07777)!=0700)return 1;}
 int fd=open(argv[3],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 1;
 struct stat info={};if(fstat(fd,&info) || !S_ISREG(info.st_mode) || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return 1;}
 FILE* file=fdopen(fd,"rb");if(!file){close(fd);return 1;}
 PioneerModelWeightCache::Digest digest={};std::string error;
 puts("QGW8_REAL_HEAD_WORKER authenticating original; SIMULATED_NO_DEVICE");fflush(stdout);
 if(!PioneerModelWeightCache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error)
  || PioneerModelWeightCache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"){fclose(file);return 1;}
 PioneerQwen38Gguf model;if(!model.Open(file) || !model.Stable()){fclose(file);return 1;}
 const auto* head=model.Find("output.weight");if(!head){fclose(file);return 1;}
 const unsigned index=unsigned(head-model.Tensors().data());
 std::array<uint32_t,5120> input={};std::array<std::vector<uint8_t>,3> exportWeights;std::vector<uint8_t> exportInput;
 std::array<std::array<uint32_t,32>,3> exportGolden={};std::array<std::string,3> exportDigest;
 if(argc==4){
  for(unsigned i=0;i<input.size();i++){
   float x=float(int((i*73)%257)-128)*0.00390625f;std::memcpy(&input[i],&x,4);
  }
 }else{
  int snapshot=open(argv[4],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);struct stat a={},b={};
  constexpr size_t bytes=(16+2*5120+1)*4;std::vector<uint32_t> words(bytes/4);
  PioneerModelWeightCache::Digest hash={};
  if(snapshot<0 || fstat(snapshot,&a) || !S_ISREG(a.st_mode) || size_t(a.st_size)!=bytes
   || !PioneerModelWeightCache::HashFile(snapshot,bytes,hash,error)
   || PioneerModelWeightCache::Hex(hash)!="ce1ed4778227d6deb4bc0fd85440facf2a680c7d60b2e7d92c60e74abd73c98a"
   || lseek(snapshot,0,SEEK_SET)<0){if(snapshot>=0)close(snapshot);fclose(file);return 1;}
  size_t done=0;while(done<bytes){ssize_t n=read(snapshot,reinterpret_cast<uint8_t*>(words.data())+done,bytes-done);if(n<=0){close(snapshot);fclose(file);return 1;}done+=size_t(n);}
  if(fstat(snapshot,&b) || a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime || close(snapshot)
   || words[0]!=0x38484351 || words[1]!=1 || words[2]!=248319 || words[3]!=64 || words[4]!=5120 || words[5]!=2
   || words[6]!=70253 || words[7]!=70253 || words.back()!=0x454e4448){fclose(file);return 1;}
  for(unsigned i=8;i<16;i++)if(words[i]){fclose(file);return 1;}
  const auto* norm=model.Find("output_norm.weight");std::vector<float> weights(5120),hidden(5120);
  if(!norm || norm->type!=0 || norm->inputs!=5120 || norm->outputs!=1
   || !model.ReadRow(unsigned(norm-model.Tensors().data()),0,weights.data(),weights.size())){fclose(file);return 1;}
  double sum=0;
  for(unsigned i=0;i<5120;i++){
   const uint32_t bits=words[16+5120+i];if((bits&0x7f800000)==0x7f800000){fclose(file);return 1;}
   std::memcpy(&hidden[i],&bits,4);sum+=double(hidden[i]*hidden[i]);
  }
  const float scale=1.f/std::sqrt(float(sum/5120)+1e-6f);
  if(!std::isfinite(scale) || !(scale>0)){fclose(file);return 1;}
  for(unsigned i=0;i<5120;i++){float x=(hidden[i]*scale)*weights[i];if(!std::isfinite(x)){fclose(file);return 1;}std::memcpy(&input[i],&x,4);}
  puts("QGW8_REAL_HEAD_WORKER pinned CPU-reference hidden normalized; NOT A CARD SNAPSHOT");fflush(stdout);
 }
 for(unsigned first:{0U,70240U,108128U,248288U}){
  std::vector<uint8_t> packed;std::array<uint32_t,32> expected={};
  if(!PioneerQ38HeadBlockFloatDiagnostic::PackBand(model,index,first,PioneerBlackholeBlockFloat::Format::Bfp8B,packed)
   || !PioneerBfp4PromptQuality::PredictFormat(PioneerBlackholeBlockFloat::Format::Bfp8B,packed.data(),packed.size(),input.data(),5120,expected)
   || !model.Stable()){fclose(file);return 1;}
  auto image=std::make_unique<HybridImage>(argv[1],argv[2],1,5120,1);
  assert(image->transfer.weights.size()==packed.size());image->transfer.weights=packed;
  for(unsigned i=0;i<5120;i++){
   const uint32_t bits=input[i];const uint16_t bf=(bits&0x7f800000)?uint16_t((bits+0x7fff+((bits>>16)&1))>>16):uint16_t((bits>>16)&0x8000);
   image->transfer.root[i*2]=uint8_t(bf);image->transfer.root[i*2+1]=uint8_t(bf>>8);
  }
  image->Boot();assert(image->pc==image->waitPC && image->regs[8]==0x70000);
  image->Put(image->regs[8]-1548,64); // pinned worker, synthetic layer completion
  auto& leaf=image->transfer.job;leaf.words[11]=1;
  uint32_t j[64]={};j[0]=HW::Magic;j[1]=1;j[2]=HW::GlobalMatrix;j[3]=image->boot[3];j[4]=image->boot[4];j[5]=64;j[7]=2025;
  for(unsigned i=0;i<8;i++)j[8+i]=image->boot[8+i];for(unsigned i=0;i<16;i++)j[16+i]=leaf.words[i];
  j[32]=first;j[33]=HW::Vocabulary;j[34]=0;j[35]=HW::JobWitness;
  assert(HW::JobShape(j,image->boot.data(),0,0));
  for(unsigned i=0;i<64;i++)image->Put(HW::Job+i*4,j[i]);image->Put(HW::Doorbell,1);
  if(!image->Finish(1) || !image->native.Clean()){fclose(file);return 1;}
  unsigned best=0;
  for(unsigned row=0;row<32;row++){
   unsigned actual=image->Get(W::Output+row*4);
   if(actual!=expected[row]){fprintf(stderr,"QGW8_REAL_HEAD_MISMATCH first%u row%u actual%08x expected%08x;SIMULATED_NO_DEVICE\n",first,row,actual,expected[row]);fclose(file);return 1;}
   if(HW::Better(actual,first+row,expected[best],first+best))best=row;
  }
  if(image->Get(HW::Partial+8*4)!=first+best || image->Get(HW::Partial+9*4)!=expected[best] || !model.Stable()){fclose(file);return 1;}
  if(argc==6 && first!=248288U){
   unsigned sample=first==0?0:first==70240?1:2;
   exportWeights[sample].assign(160*1104,0);
   for(unsigned tile=0;tile<160;tile++)std::memcpy(exportWeights[sample].data()+tile*1104+16,packed.data()+tile*1088,1088);
   std::vector<uint8_t> wire(160*80,0);
   for(unsigned tile=0;tile<160;tile++)std::memcpy(wire.data()+tile*80+16,image->transfer.root.data()+tile*64,64);
   if(sample && wire!=exportInput){fclose(file);return 1;}exportInput=std::move(wire);
   exportGolden[sample]=expected;std::vector<uint8_t> joined=exportWeights[sample];joined.insert(joined.end(),exportInput.begin(),exportInput.end());
   exportDigest[sample]=PioneerModelWeightCache::Hex(PioneerModelWeightCache::Hash(joined.data(),joined.size()));
  }
  printf("QGW8_REAL_HEAD_WORKER first%u rows32 exact32 argmax%u;SIMULATED_NO_DEVICE\n",first,first+best);fflush(stdout);
 }
 if(argc==6){
  const std::string dir=argv[5];FILE* data=NewPrivateFile(dir+"/PioneerNativeBfpModelBandData.h");if(!data){fclose(file);return 1;}
  bool ok=fputs("/* PRIVATE MODEL WEIGHTS: DO NOT COMMIT. */\n#ifndef PIONEER_NATIVE_BFP_MODEL_BAND_DATA_H\n#define PIONEER_NATIVE_BFP_MODEL_BAND_DATA_H\n#include <stdint.h>\nstruct PioneerNativeBfpModelBandData {\n static uint32_t Word(unsigned sample,unsigned width,unsigned address){\n if(sample>=3 || width!=5120 || (address&3))return 0;\n",data)>=0;
  for(unsigned s=0;s<3;s++){char name[8];snprintf(name,sizeof(name),"w%u",s);ok=WriteWords(data,name,exportWeights[s]) && ok;}
  ok=WriteWords(data,"input",exportInput) && ok;
  ok=(fputs(" if(address>=0x8fff0 && address<0x8fff0+176640){const unsigned i=(address-0x8fff0)/4;return sample==0?w0[i]:sample==1?w1[i]:w2[i];}\n if(address>=0x123ff0 && address<0x123ff0+12800)return input[(address-0x123ff0)/4];\n return 0;}\n};\n#endif\n",data)>=0) && ok;
  if(!FinishPrivate(data) || !ok){fclose(file);return 1;}
  FILE* meta=NewPrivateFile(dir+"/PioneerNativeBfpModelBandMetadata.h");if(!meta){fclose(file);return 1;}
  ok=fprintf(meta,"/* PRIVATE DIAGNOSTIC: not a device result. */\n#ifndef PIONEER_NATIVE_BFP_MODEL_BAND_METADATA_H\n#define PIONEER_NATIVE_BFP_MODEL_BAND_METADATA_H\n#include <stdint.h>\nstruct PioneerNativeBfpModelBandMetadata {\n static constexpr unsigned Width=5120,Rows=248320,Tensor=%u,Samples=3;\n static unsigned First(unsigned s){const unsigned r[3]={0,70240,108128};return s<3?r[s]:~0U;}\n static const uint32_t* Golden(unsigned sample){\n",index)>=0;
  for(unsigned s=0;s<3;s++){if(fprintf(meta," static const uint32_t g%u[32]={",s)<0)ok=false;for(unsigned row=0;row<32;row++)if(fprintf(meta,"0x%08x%s",exportGolden[s][row],row==31?"};\n":",")<0)ok=false;}
  ok=(fputs(" return sample==0?g0:sample==1?g1:sample==2?g2:nullptr;}\n static const char* PayloadSHA(unsigned sample){\n",meta)>=0) && ok;
  for(unsigned s=0;s<3;s++)if(fprintf(meta," if(sample==%u)return \"%s\";\n",s,exportDigest[s].c_str())<0)ok=false;
  ok=(fputs(" return nullptr;}\n};\n#endif\n",meta)>=0) && ok;
  if(!FinishPrivate(meta) || !ok || !model.Stable()){fclose(file);return 1;}
  puts("QGW8_REAL_HEAD_PRIVATE_EXPORT samples3;PRIVATE_WEIGHTS_NOT_FOR_GIT_NO_DEVICE");
 }
 printf("PASS_REAL_HEAD_QGW8_WORKER 128 real-weight outputs; %s_SIMULATED_FABRIC_NOT_DEVICE_OR_SPEED\n",argc>=5?"PINNED_CPU_REFERENCE_INPUT":"SYNTHETIC_INPUT");
 return fclose(file)?1:0;
}
