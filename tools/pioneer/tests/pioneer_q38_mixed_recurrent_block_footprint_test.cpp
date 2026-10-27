/* SPDX-License-Identifier: MIT */
// Exact immutable QRB1/QMW1 code and L1 DATA geometry only. NOT a journal,
// native lifetime/STOP/HOLD/restore/disposal or original arithmetic proof.
#include "../PioneerQ38MixedRecurrentBlockPrograms.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLiveFootprint.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
static std::vector<unsigned char> Bytes(const char* directory,const char* name){
 std::ifstream f(std::string(directory)+"/"+name+".bin",std::ios::binary);assert(f.good());return {std::istreambuf_iterator<char>(f),{}};
}
int main(int argc,char** argv){
 assert(argc==3);unsigned saved=0,code=0;
 const char* root[]={"boot","text","conv"};
 const char* peer[]={"boot","mixed","dense","ffn","service","affine","text","gdn"};
 for(unsigned w=0;w<120;w++){
  unsigned words=0,last=0;
  for(unsigned n=0;n<PioneerQ38RecurrentBlockSnapshotSpans(w);n++){
   const auto s=PioneerQ38RecurrentBlockSnapshotSpan(w,n);
   assert(s.words && !(s.address&3) && s.address>=last && s.address+s.words*4<=0x180000);
   for(unsigned at=0;at<s.words;at++){
    assert(PioneerQ38RecurrentBlockSnapshotAddress(w,words)==s.address+at*4 && PioneerQ38RecurrentBlockSnapshotContains(w,s.address+at*4));words++;
   }
   last=s.address+s.words*4;
  }
  assert(words==PioneerQ38RecurrentBlockSnapshotWords(w) && PioneerQ38RecurrentBlockSnapshotBase(w)==saved
   && PioneerQ38RecurrentBlockSnapshotAddress(w,words)==UINT32_MAX);saved+=words;
  unsigned emitted=0;
  for(unsigned n=0;n<PioneerQ38RecurrentBlockCodeSpans(w);n++){
   const auto raw=Bytes(w?argv[2]:argv[1],w?peer[n]:root[n]);const auto s=PioneerQ38RecurrentBlockCodeSpan(w,n);
   assert(raw.size()==s.words*4);
   for(unsigned at=0;at<s.words;at++){
    unsigned a=s.address+at*4,value=0,off=at*4;
    assert(PioneerQ38RecurrentBlockCodeAddress(w,emitted)==a && PioneerQ38RecurrentBlockCodeContains(w,a)
     && PioneerQ38RecurrentBlockSnapshotContains(w,a) && PioneerQ38RecurrentBlockProgramWord(w,a,value));
    assert(value==(raw[off]|(uint32_t(raw[off+1])<<8)|(uint32_t(raw[off+2])<<16)|(uint32_t(raw[off+3])<<24)));emitted++;
   }
  }
  assert(emitted==PioneerQ38RecurrentBlockCodeWords(w) && PioneerQ38RecurrentBlockCodeAddress(w,emitted)==UINT32_MAX);code+=emitted;
  for(unsigned a:{0U,0x6001U,0x6004U,0x6e000U,0x75000U,0x17ffc0U,0xffb20000U,UINT32_MAX}){
   uint32_t untouched=0xaabbccdd;assert(!PioneerQ38RecurrentBlockProgramWord(w,a,untouched) && untouched==0xaabbccdd);
  }
 }
 assert(saved==TT_Q38_RECURRENT_BLOCK_SAVED_WORDS && code==TT_Q38_RECURRENT_BLOCK_CODE_WORDS);
 assert(!PioneerQ38MixedLiveSnapshotContains(0,0x64000) && PioneerQ38RecurrentBlockSnapshotContains(0,0x64000));
 for(unsigned a:{0x60000U,0x63ffcU,0x6c000U,0x6dffcU})assert(!PioneerQ38RecurrentBlockSnapshotContains(0,a));
 for(unsigned a:{0x64000U,0x65d48U,0x6bffcU,0x6e000U,0x6fffcU,0x75000U,0x75600U,0xb0000U,0xbc000U,0xc2000U,0xc8000U,
  0xd6000U,0xdb000U,0xe0000U,0xf2000U,0xfad7cU,0x100000U,0x12a000U,0x156000U,0x15b000U,0x160000U,0x17f7fcU})
  assert(PioneerQ38RecurrentBlockSnapshotContains(0,a));
 for(unsigned w:{120U,UINT32_MAX}){
  uint32_t untouched=0xaabbccdd;
  assert(!PioneerQ38RecurrentBlockSnapshotSpans(w) && !PioneerQ38RecurrentBlockCodeSpans(w)
   && !PioneerQ38RecurrentBlockSnapshotWords(w) && !PioneerQ38RecurrentBlockCodeWords(w)
   && PioneerQ38RecurrentBlockSnapshotBase(w)==UINT32_MAX && !PioneerQ38RecurrentBlockSnapshotContains(w,0x6000)
   && !PioneerQ38RecurrentBlockProgramWord(w,0x6000,untouched) && untouched==0xaabbccdd);
 }
 printf("PASS QRB1+QMW1 ALL11 signed code sections/words%u/ALL120 snapshot words%u bytes%u/separate64000Conv window+padding/NO old dense journal alias;DATA geometry ONLY,NOT native lifecycle/device/Work authority\n",code,saved,saved*4);
}
