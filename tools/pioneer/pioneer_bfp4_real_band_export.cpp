/* SPDX-License-Identifier: MIT */
// CPU-only PRIVATE diagnostic samples. Never touches the GPU or model cache.
// Output is lossy converted model data: do not commit or redistribute it.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error BFP4 real-band export is a separate CPU diagnostic,never Work or validation
#endif
#include "PioneerQ38NativeWeightTiles.h"
#include "PioneerModelWeightCache.h"
#include "PioneerBlackholeBf16Arithmetic.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
namespace {
using Cache=PioneerModelWeightCache;
using Codec=PioneerBlackholeBlockFloat;
void Require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void Append(std::vector<uint8_t>& data,uint32_t value){for(unsigned i=0;i<4;i++)data.push_back(uint8_t(value>>(8*i)));}
float Float(uint32_t bits){float value;memcpy(&value,&bits,4);return value;}
void Save(const std::string& path,const std::vector<uint8_t>& data){
 int fd=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
 Require(fd>=0,"private output exists or cannot be created");
 size_t done=0;while(done<data.size()){
  ssize_t n=write(fd,data.data()+done,data.size()-done);
  if(n<=0){close(fd);throw std::runtime_error("private output write failed");}done+=size_t(n);
 }
 bool okay=!fsync(fd);okay=(!close(fd)) && okay;Require(okay,"private output sync or close failed");
}
int Run(int argc,char** argv){
 if(argc!=3){fputs("Use ORIGINAL_GGUF NEW_PRIVATE_OUTPUT_DIRECTORY;CPU-only.\n",stderr);return 2;}
 struct stat directory={};Require(!lstat(argv[2],&directory) && S_ISDIR(directory.st_mode) && directory.st_uid==geteuid()
  && (directory.st_mode&07777)==0700,"output directory must be private,owned and not a symlink");
 std::unique_ptr<FILE,decltype(&fclose)> file(fopen(argv[1],"rb"),&fclose);Require(bool(file),"cannot open original model");
 PioneerQwen38Gguf model;Require(model.Open(file.get()),"complete original model layout refused");auto epoch=model.Epoch();
 Cache::Digest digest{};std::string error;
 puts("AUTHENTICATING_SAME_OPEN_COMPLETE_ORIGINAL;CPU_ONLY_NO_GPU");fflush(stdout);
 Require(Cache::HashFile(fileno(file.get()),PioneerQwen38Gguf::FullBytes,digest,error)
  && Cache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747"
  && model.Stable() && model.Epoch()==epoch,"original model hash or identity changed");
 const auto* tensor=model.Find("blk.0.ffn_gate.weight");
 Require(tensor && tensor->inputs==5120 && tensor->outputs==17408 && tensor->type==12,"pinned Q4 tensor shape refused");
 unsigned index=unsigned(tensor-model.Tensors().data());Require(index==7,"pinned tensor ordinal changed");
 const unsigned firsts[3]={0,8704,17376};
 for(unsigned sample=0;sample<3;sample++){
  std::vector<uint8_t> data;data.reserve(16+160*576+5120*2+32*4);
  Append(data,0x34504252);Append(data,1);Append(data,5120);Append(data,firsts[sample]); // RBP4
  std::array<uint32_t,32> golden={};std::array<long double,32> originalDot={},convertedDot={};
  long double originalEnergy=0,errorEnergy=0;
  for(unsigned tile=0;tile<160;tile++){
   std::array<float,1024> original={},decoded={};std::array<uint8_t,576> packed={};
   Require(PioneerQ38NativeWeightTiles::ReadFloatTile(model,index,uint64_t(firsts[sample]/32)*160+tile,original.data(),1024),"original tile read failed");
   Require(Codec::PackTile(Codec::Format::Bfp4B,original.data(),1024,packed.data(),576)
    && Codec::UnpackTile(Codec::Format::Bfp4B,packed.data(),576,decoded.data(),1024),"BFP4 conversion refused");
   data.insert(data.end(),packed.begin(),packed.end());
   for(unsigned row=0;row<32;row++)for(unsigned half=0;half<2;half++){
    uint16_t a[16],b[16];
    for(unsigned c=0;c<16;c++){
     unsigned column=half*16+c,position=row*32+column;uint32_t bits=0;memcpy(&bits,&decoded[position],4);
     Require(!(bits&65535),"converted weight is not exactly BF16");a[c]=uint16_t(bits>>16);
     unsigned input=tile*32+column;b[c]=uint16_t(0x3f00+(input*13)%256+((input&3)==2?0x8000:0));
     const long double activation=Float(uint32_t(b[c])<<16),orig=original[position],converted=decoded[position];
     originalDot[row]+=orig*activation;convertedDot[row]+=converted*activation;
     originalEnergy+=orig*orig;const long double difference=converted-orig;errorEnergy+=difference*difference;
    }
    for(unsigned phase:{0U,2U})Require(PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,golden[row],golden[row]),"documented arithmetic refused");
   }
  }
  for(unsigned i=0;i<5120;i++){uint16_t b=uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0));data.push_back(uint8_t(b));data.push_back(uint8_t(b>>8));}
  for(auto word:golden)Append(data,word);
  Require(data.size()==102544 && model.Stable() && model.Epoch()==epoch,"source changed before private sample publication");
  long double dotEnergy=0,dotError=0;
  for(unsigned row=0;row<32;row++){dotEnergy+=originalDot[row]*originalDot[row];long double d=convertedDot[row]-originalDot[row];dotError+=d*d;}
  Require(originalEnergy>0 && dotEnergy>0,"degenerate diagnostic input or model sample");
  char name[64];snprintf(name,sizeof(name),"/band%u.rbp4",sample);Save(std::string(argv[2])+name,data);
  printf("BFP4_REAL_BAND sample%u first%u K5120 weights92160 input10240 expected32 sha256%s weight_relative_l2%.9f dot_relative_l2%.9f;DIAGNOSTIC_ONLY_NOT_MODEL_QUALITY\n",
   sample,firsts[sample],Cache::Hex(Cache::Hash(data.data(),data.size())).c_str(),double(std::sqrt(errorEnergy/originalEnergy)),double(std::sqrt(dotError/dotEnergy)));
  fflush(stdout);
 }
 Require(model.Stable() && model.Epoch()==epoch,"source changed during export;discard private output");
 puts("PASS_PRIVATE_BFP4_REAL_BAND_EXPORT samples3 source_authenticated;NO GPU,MODEL QUALITY OR SPEED CLAIM");return 0;
}
}
int main(int argc,char** argv){try{return Run(argc,argv);}catch(const std::exception& e){fprintf(stderr,"BFP4_EXPORT_REFUSED: %s;PRIVATE OUTPUT MAY BE INCOMPLETE\n",e.what());return 1;}}
