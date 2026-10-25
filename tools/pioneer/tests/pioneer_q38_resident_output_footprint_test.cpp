/* SPDX-License-Identifier: MIT */
// Pure fixed geometry + actual blob lengths. NO native/IO/owner capability.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputFootprint.h"
static unsigned Length(const std::string& path){std::ifstream f(path,std::ios::binary|std::ios::ate);assert(f);auto n=f.tellg();assert(n>0 && n<=65536 && n%4==0);return unsigned(n);}
int main(int argc,char** argv){
 assert(argc==2);unsigned total=0,codeTotal=0;
 const char* root[]={"boot","conv","matrix","text","cohort"};
 const char* peer[]={"boot","ffn","service","affine","text","gdn"};
 for(unsigned w=0;w<120;w++){
  assert(PioneerQ38ResidentOutputSnapshotBase(w)==total);
  unsigned words=0;
  for(unsigned i=0;i<PioneerQ38ResidentOutputSpanCount(w);i++){
   auto s=PioneerQ38ResidentOutputSnapshotSpan(w,i);assert(s.words && !(s.address&3) && s.address<0x180000 && s.words*4<=0x180000-s.address);
   for(unsigned j=0;j<i;j++){
    auto a=PioneerQ38ResidentOutputSnapshotSpan(w,j);assert(s.address>=a.address+a.words*4 || a.address>=s.address+s.words*4);
   }
   for(unsigned n=0;n<s.words;n++){
    unsigned at=s.address+n*4;assert(PioneerQ38ResidentOutputSnapshotAddress(w,words+n)==at && PioneerQ38ResidentOutputSnapshotContains(w,at));
   }
   words+=s.words;
  }
  assert(words==PioneerQ38ResidentOutputSnapshotWords(w) && PioneerQ38ResidentOutputSnapshotAddress(w,words)==UINT32_MAX);
  total+=words;unsigned codeWords=0;
  for(unsigned i=0;i<PioneerQ38ResidentOutputCodeSpanCount(w);i++){
   auto s=PioneerQ38ResidentOutputCodeSpan(w,i);
   auto path=std::string(argv[1])+(w?"/exact-companion/":"/")+(w?peer[i]:root[i])+".bin";
   assert(Length(path)==s.words*4);codeWords+=s.words;
   for(unsigned n=0;n<s.words;n++)assert(PioneerQ38ResidentOutputSnapshotContains(w,s.address+n*4) && PioneerQ38ResidentOutputProgramImmutable(w,s.address+n*4));
  }
  assert(codeWords==PioneerQ38ResidentOutputCodeWords(w));codeTotal+=codeWords;
  for(uint32_t bad:{0U,0x6001U,0x63ffcU,0x6dfffU,0xac000U,0xb7ffcU,0xc8000U,0x17ffe0U,0x180000U,0xffb20000U,0xffef0000U,UINT32_MAX}){
   if(!w && bad==0xc8000)continue;assert(!PioneerQ38ResidentOutputSnapshotContains(w,bad));
  }
  assert(!PioneerQ38ResidentOutputProgramImmutable(w,0x6e000) && !PioneerQ38ResidentOutputProgramImmutable(w,0x70700)
   && !PioneerQ38ResidentOutputProgramImmutable(w,0x74000) && !PioneerQ38ResidentOutputProgramImmutable(w,0x17ffc0));
  auto bad=PioneerQ38ResidentOutputSnapshotSpan(w,PioneerQ38ResidentOutputSpanCount(w));assert(bad.address==UINT32_MAX && !bad.words);
 }
 assert(total==10951352 && codeTotal==TT_Q38_RESIDENT_OUTPUT_CODE_WORDS && total*4==43805408);
 for(unsigned bad:{120U,121U,UINT32_MAX}){
  assert(!PioneerQ38ResidentOutputSpanCount(bad) && !PioneerQ38ResidentOutputCodeSpanCount(bad) && !PioneerQ38ResidentOutputSnapshotWords(bad)
   && !PioneerQ38ResidentOutputCodeWords(bad) && PioneerQ38ResidentOutputSnapshotBase(bad)==UINT32_MAX
   && PioneerQ38ResidentOutputSnapshotAddress(bad,0)==UINT32_MAX && !PioneerQ38ResidentOutputSnapshotContains(bad,0x6000));
 }
 printf("PASS NEW fixed ALL120 footprint/%u arbitrary-original words/%u code words/ALL code blob lengths+every snapshot word bounded and disjoint/MMIO+GDDR+bad ordinals refused;DATA ONLY,NO owner/write/restore/launch grant\n",total,codeTotal);
 return 0;
}
