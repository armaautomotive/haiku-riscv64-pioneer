/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_NATIVE_BFP4_DRAM_MODEL_SOURCE_H
#define PIONEER_NATIVE_BFP4_DRAM_MODEL_SOURCE_H
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || defined(__FAST_MATH__)
#error BFP4 source reference is a separate CPU diagnostic, never Work or validation
#endif
#include "PioneerModelWeightCache.h"
#include "PioneerQ38NativeWeightTiles.h"
#include "PioneerBlackholeBf16Arithmetic.h"
#include "PioneerQ38Bfp4BandInput.h"
#include "PioneerNativeBfp4DramModelMetadata.h" // PRIVATE predictions
#if defined(PIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC) && PIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC != 1
#error The matched-input BFP4 diagnostic role must be explicitly 1
#endif
#include <array>
#include <cfenv>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

// Authenticate the same open original, regenerate all three exact fixed
// samples and their CPU debug predictions, then keep the source open. No
// writes, GPU access, inference fallback, or model-quality qualification.
class PioneerNativeBfp4DramModelSource {
 using Cache=PioneerModelWeightCache;
 using Meta=PioneerNativeBfp4DramModelMetadata;
 std::unique_ptr<FILE,decltype(&fclose)> file{nullptr,&fclose};
 PioneerQwen38Gguf model;uint64_t epoch=0;bool attempted=false,prepared=false;
 static bool Fail(std::string& error,const char* why){error=why;return false;}
 bool Stable()const{return file && model.Ready() && model.Epoch()==epoch && model.Stable();}
 static void Append(std::vector<uint8_t>& data,uint32_t value){for(unsigned i=0;i<4;i++)data.push_back(uint8_t(value>>(8*i)));}
public:
 bool Ready()const{return prepared && Stable();}
 bool Prepare(const char* path,std::string& error){
  if(attempted)return Fail(error,"Source preparation is one-shot.");
  attempted=true;
  if(!path || std::fegetround()!=FE_TONEAREST)return Fail(error,"Original path/default rounding required.");
  int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return Fail(error,"Original read-only open refused.");
  struct stat info={};
  if(fstat(fd,&info) || !S_ISREG(info.st_mode) || info.st_nlink!=1 || info.st_uid!=geteuid() || (info.st_mode&0022)
   || uint64_t(info.st_size)!=PioneerQwen38Gguf::FullBytes){close(fd);return Fail(error,"Original identity/permissions/size refused.");}
  FILE* stream=fdopen(fd,"rb");if(!stream){close(fd);return Fail(error,"Original stream refused.");}file.reset(stream);
  if(!model.Open(file.get()))return Fail(error,"Original model layout refused.");
  epoch=model.Epoch();
  Cache::Digest digest={};
  if(!Cache::HashFile(fd,PioneerQwen38Gguf::FullBytes,digest,error) || !Stable()
   || Cache::Hex(digest)!="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747")return Fail(error,"Original hash or stability refused.");
  const auto* tensor=model.Find("blk.0.ffn_gate.weight");
  if(!tensor || tensor->inputs!=5120 || tensor->outputs!=17408 || tensor->type!=12 || unsigned(tensor-model.Tensors().data())!=7)
   return Fail(error,"Pinned learned matrix identity changed.");
  const unsigned firsts[3]={0,8704,17376};
#if defined(PIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC)
  constexpr bool matchedQ4=true;
  const char* pins[3]={"916ef0df61974947e4eff96294bb4e306ec8aa1400f2db22699bd3a0e6482eb0",
   "9b5630aa0cb72812452e6d194b7e3e3108cee0e72f593f0c63ef72d1cf75bb41",
   "065fdac4a43dd744844e4ee2030ca2db44e4c822b7bc9cf68e540839c8c40745"};
#else
  constexpr bool matchedQ4=false;
  const char* pins[3]={"edbc6d7cadcd85d0114ff16a5c44afb3627296497a7a0e03f76efa41da5ac7dd",
   "59f05959ef8486021eefd0b7e02e083b6f18a7eef747f15d5e24e6e87c587695",
   "036bcee72fc9bb84fedb9c6f93a6d22358f9d7c90c610cc5c19159f002a587e3"};
#endif
  for(unsigned sample=0;sample<3;sample++){
   std::vector<uint8_t> data;data.reserve(102544);Append(data,0x34504252);Append(data,1);Append(data,5120);Append(data,firsts[sample]);
   std::array<uint32_t,32> expected={};
   for(unsigned tile=0;tile<160;tile++){
    std::array<float,1024> original={},decoded={};std::array<uint8_t,576> packed={};
    if(!Stable() || !PioneerQ38NativeWeightTiles::ReadFloatTile(model,7,uint64_t(firsts[sample]/32)*160+tile,original.data(),1024)
     || !PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp4B,original.data(),1024,packed.data(),576)
     || !PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp4B,packed.data(),576,decoded.data(),1024) || !Stable())
     return Fail(error,"Fresh tile/source refused.");
    data.insert(data.end(),packed.begin(),packed.end());
    for(unsigned row=0;row<32;row++)for(unsigned half=0;half<2;half++){
     uint16_t a[16],b[16];
     for(unsigned k=0;k<16;k++){
      unsigned column=half*16+k,input=tile*32+column;uint32_t bits=0;memcpy(&bits,&decoded[row*32+column],4);
      if(bits&65535)return Fail(error,"Converted weight is not BF16.");
      a[k]=uint16_t(bits>>16);
      b[k]=PioneerQ38Bfp4BandInput::Word(input,matchedQ4);
     }
     for(unsigned phase:{0U,2U})if(!PioneerBlackholeBf16Arithmetic::Instruction(a,b,phase,expected[row],expected[row]))return Fail(error,"Reference arithmetic refused.");
    }
   }
   for(unsigned i=0;i<5120;i++){uint16_t b=PioneerQ38Bfp4BandInput::Word(i,matchedQ4);data.push_back(uint8_t(b));data.push_back(uint8_t(b>>8));}
   for(auto word:expected)Append(data,word);
   const auto* golden=Meta::Golden(sample);
   if(data.size()!=102544 || !golden || Cache::Hex(Cache::Hash(data.data(),data.size()))!=pins[sample]
    || memcmp(expected.data(),golden,128) || !Stable())return Fail(error,"Authenticated sample or reference mismatch.");
  }
  if(!Stable())return Fail(error,"Original changed before reference publication.");
  prepared=true;error.clear();return true;
 }
};
#endif
