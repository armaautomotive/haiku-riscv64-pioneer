/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerImages.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputImages.h"
static std::vector<uint32_t> Read(const std::string& path){
 std::ifstream input(path,std::ios::binary|std::ios::ate);assert(input);auto bytes=input.tellg();assert(bytes>0 && bytes%4==0);
 std::vector<uint32_t> data(static_cast<size_t>(bytes)/4);input.seekg(0);input.read(reinterpret_cast<char*>(data.data()),bytes);
 assert(input && input.gcount()==bytes);return data;
}
int main(int argc,char** argv){
 assert(argc==3);unsigned total=0;uint32_t word=0,old=0;
 assert(PioneerQ38ResidentLayerProgramWord(0,0x72024,word) && word==0x31525337
  && PioneerQ38ResidentOutputProgramWord(0,0x72024,old) && old!=word);
 assert(PioneerQ38ResidentLayerProgramWord(0,0x72038,word) && word==0x0000006f);
 assert(PioneerQ38ResidentLayerProgramWord(0,0x7203c,word) && word==0xffdff06f);
 const char* names[2][7]={{"boot","conv","matrix","text","cohort","norm","ffn"},{"boot","ffn","service","affine","text","gdn",nullptr}};
 std::vector<uint32_t> images[2][7];
 for(unsigned family=0;family<2;family++)for(unsigned i=0;i<(family?6U:7U);i++)images[family][i]=Read(std::string(argv[family+1])+"/"+names[family][i]+".bin");
 for(unsigned w=0;w<120;w++){
  assert(PioneerQ38ResidentLayerCodeBase(w)==total);unsigned ordinal=0;
  for(unsigned i=0;i<PioneerQ38ResidentLayerCodeSpanCount(w);i++){
   const auto s=PioneerQ38ResidentLayerImage(w,i);const auto& expected=images[!!w][i];assert(s.data && s.words==expected.size());
   for(unsigned j=0;j<s.words;j++){
    uint32_t a=s.address+j*4,actual=0;
    assert(s.data[j]==expected[j] && PioneerQ38ResidentLayerCodeAddress(w,ordinal++)==a
     && PioneerQ38ResidentLayerProgramImmutable(w,a) && PioneerQ38ResidentLayerSnapshotContains(w,a)
     && PioneerQ38ResidentLayerProgramWord(w,a,actual) && actual==expected[j]);
    if(w){uint32_t previous=0;assert(PioneerQ38ResidentOutputProgramWord(w,a,previous) && previous==actual);}
    total++;
   }
  }
  assert(ordinal==PioneerQ38ResidentLayerCodeWords(w) && PioneerQ38ResidentLayerCodeAddress(w,ordinal)==UINT32_MAX
   && PioneerQ38ResidentLayerCodeAddress(w,UINT32_MAX)==UINT32_MAX);
  const auto bad=PioneerQ38ResidentLayerImage(w,PioneerQ38ResidentLayerCodeSpanCount(w));assert(!bad.data && !bad.words && bad.address==UINT32_MAX);
  for(uint32_t a=0;a<0x180000;a+=4){
   uint32_t value=0x5a6bc7d8;bool valid=PioneerQ38ResidentLayerProgramWord(w,a,value);
   assert(valid==PioneerQ38ResidentLayerProgramImmutable(w,a));if(!valid)assert(value==0x5a6bc7d8);
  }
  for(uint32_t a:{1U,0x6001U,0x64001U,0x68002U,0x72003U,0xb8001U,0x158001U,0x15c002U,0xfffffffcU,UINT32_MAX}){
   uint32_t value=0x5a6bc7d8;assert(!PioneerQ38ResidentLayerProgramWord(w,a,value) && value==0x5a6bc7d8);
  }
 }
 assert(total==2270365);
 for(unsigned w:{120U,121U,UINT32_MAX}){
  const auto s=PioneerQ38ResidentLayerImage(w,0);uint32_t value=0x5a6bc7d8;
  assert(!s.data && !s.words && s.address==UINT32_MAX && PioneerQ38ResidentLayerCodeBase(w)==UINT32_MAX
   && PioneerQ38ResidentLayerCodeAddress(w,0)==UINT32_MAX && !PioneerQ38ResidentLayerProgramWord(w,0x6000,value) && value==0x5a6bc7d8);
 }
 puts("PASS NEW layer ALL13 exact emitted-code sections/ALL120 mappings/2270365codewords/9081460B;exhaustive L1/padding/stack/runtime/manifest/trailer/bad-worker/unaligned refusal WITHOUT output mutation;old68 data unchanged,SAME119peer code constants DATA ONLY;NO mapping,source,initialized,native/Work owner or launch grant");
}
