/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputImages.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputProof.h"
static std::vector<uint32_t> Read(const std::string& path){
 std::ifstream input(path,std::ios::binary|std::ios::ate);assert(input);
 const auto bytes=input.tellg();assert(bytes>0 && bytes%4==0);
 std::vector<uint32_t> data(static_cast<size_t>(bytes)/4);input.seekg(0);
 input.read(reinterpret_cast<char*>(data.data()),bytes);assert(input && input.gcount()==bytes);return data;
}
int main(int argc,char** argv){
 assert(argc==3);unsigned total=0;
 uint32_t word=0;
 assert(PioneerQ38ResidentOutputProgramWord(0,0x72038,word) && word==0x0000006f);
 assert(PioneerQ38ResidentOutputProgramWord(0,0x7203c,word) && word==0xffdff06f && word!=0x001802b7); // NOT old fault entry
 assert(PioneerQ38ResidentOutputProgramWord(0,0x72060,word) && word==0x001802b7);
 for(uint32_t pc=0x72000;pc<0x72084;pc++){
  assert(PioneerQ38ResidentOutputRootTerminalPc(pc)==(pc==0x72038 || pc==0x7203c));
  if(pc>=0x72040 && pc<0x72080)assert(PioneerQ38ResidentOutputRootRejectedPc(pc));
 }
 const char* root[]={"boot","conv","matrix","text","cohort"};
 const char* peer[]={"boot","ffn","service","affine","text","gdn"};
 std::vector<uint32_t> images[2][6];
 for(unsigned family=0;family<2;family++)for(unsigned i=0;i<(family?6U:5U);i++)
  images[family][i]=Read(std::string(argv[family+1])+"/"+(family?peer[i]:root[i])+".bin");
 for(unsigned w=0;w<120;w++){
  assert(PioneerQ38ResidentOutputCodeBase(w)==total);unsigned ordinal=0;
  for(unsigned i=0;i<PioneerQ38ResidentOutputCodeSpanCount(w);i++){
   const auto s=PioneerQ38ResidentOutputImage(w,i);const auto& expected=images[w?1:0][i];
   assert(s.data && s.words==expected.size());
   for(unsigned j=0;j<s.words;j++){
    const uint32_t a=s.address+j*4;uint32_t actual=0;
    assert(s.data[j]==expected[j] && PioneerQ38ResidentOutputCodeAddress(w,ordinal++)==a);
    assert(PioneerQ38ResidentOutputProgramImmutable(w,a) && PioneerQ38ResidentOutputSnapshotContains(w,a));
    assert(PioneerQ38ResidentOutputProgramWord(w,a,actual) && actual==expected[j]);total++;
   }
  }
  assert(ordinal==PioneerQ38ResidentOutputCodeWords(w));
  assert(PioneerQ38ResidentOutputCodeAddress(w,ordinal)==UINT32_MAX
   && PioneerQ38ResidentOutputCodeAddress(w,UINT32_MAX)==UINT32_MAX);
  const auto bad=PioneerQ38ResidentOutputImage(w,PioneerQ38ResidentOutputCodeSpanCount(w));
  assert(bad.address==UINT32_MAX && !bad.data && !bad.words);
  // Exhaustive ALL L1 words, including every arbitrary snapshot gap and
  // stack/manifest/runtime/trailer word, receive no manufactured code.
  for(uint32_t a=0;a<0x180000;a+=4){
   uint32_t value=0x5a6bc7d8;const bool valid=PioneerQ38ResidentOutputProgramWord(w,a,value);
   assert(valid==PioneerQ38ResidentOutputProgramImmutable(w,a));
   if(!valid)assert(value==0x5a6bc7d8);
  }
  for(uint32_t a:{1U,0x6001U,0x64001U,0x68002U,0x72003U,0xb8001U,0xfffffffcU,UINT32_MAX}){
   uint32_t value=0x5a6bc7d8;assert(!PioneerQ38ResidentOutputProgramWord(w,a,value) && value==0x5a6bc7d8);
  }
 }
 assert(total==TT_Q38_RESIDENT_OUTPUT_CODE_WORDS);
 for(unsigned w:{120U,121U,UINT32_MAX}){
  const auto s=PioneerQ38ResidentOutputImage(w,0);uint32_t value=0x5a6bc7d8;
  assert(!s.data && !s.words && s.address==UINT32_MAX && PioneerQ38ResidentOutputCodeBase(w)==UINT32_MAX);
  assert(PioneerQ38ResidentOutputCodeAddress(w,0)==UINT32_MAX
   && !PioneerQ38ResidentOutputProgramWord(w,0x6000,value) && value==0x5a6bc7d8);
 }
 std::printf("PASS resident-output ALL11 exact emitted-image sections/ALL120 mappings/2259580 code words/9038320 bytes; every L1 word checked, padding+stacks+runtime+MMIO+bad worker+unaligned refused without output mutation; code DATA only,NO native owner or hardware grant\n");
}
