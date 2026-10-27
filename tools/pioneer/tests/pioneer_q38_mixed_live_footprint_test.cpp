/* SPDX-License-Identifier: MIT */
// Exact signed code/complete L1 GEOMETRY,NOT a native journal/owner test.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38MixedLivePrograms.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerFootprint.h"
#include "../PioneerQ38MixedLiveBlockRootLayout.h"
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
 assert(argc==3);unsigned snapshots=0,code=0;
 const char* peer[]={"boot","mixed","dense","ffn","service","affine","text","gdn"};
 for(unsigned w=0;w<120;w++){
  unsigned words=0,last=0;
  for(unsigned i=0;i<PioneerQ38MixedLiveSnapshotSpans(w);i++){
   const auto s=PioneerQ38MixedLiveSnapshotSpan(w,i);assert(s.words && !(s.address&3) && s.address>=last && s.address+s.words*4<=0x180000);
   for(unsigned at=0;at<s.words;at++){
    assert(PioneerQ38MixedLiveSnapshotAddress(w,words)==s.address+at*4 && PioneerQ38MixedLiveSnapshotContains(w,s.address+at*4));words++;
   }
   last=s.address+s.words*4;
  }
  assert(words==PioneerQ38MixedLiveSnapshotWords(w) && PioneerQ38MixedLiveSnapshotBase(w)==snapshots);snapshots+=words;
  assert(PioneerQ38MixedLiveSnapshotAddress(w,words)==UINT32_MAX);
  unsigned codeWords=0;
  for(unsigned i=0;i<PioneerQ38MixedLiveCodeSpans(w);i++){
   auto raw=Bytes(w?argv[2]:argv[1],w?peer[i]:i?"code":"boot");const auto s=PioneerQ38MixedLiveCodeSpan(w,i);assert(raw.size()==s.words*4);
   for(unsigned at=0;at<s.words;at++){
    unsigned address=s.address+at*4,value=0;assert(PioneerQ38MixedLiveCodeAddress(w,codeWords)==address
     && PioneerQ38MixedLiveSnapshotContains(w,address) && PioneerQ38MixedLiveCodeContains(w,address)
     && PioneerQ38MixedLiveProgramWord(w,address,value));
    unsigned off=at*4,wanted=raw[off]|(uint32_t(raw[off+1])<<8)|(uint32_t(raw[off+2])<<16)|(uint32_t(raw[off+3])<<24);
    assert(value==wanted);codeWords++;
   }
  }
  assert(codeWords==PioneerQ38MixedLiveCodeWords(w) && PioneerQ38MixedLiveCodeAddress(w,codeWords)==UINT32_MAX);code+=codeWords;
  for(unsigned address:{0U,0x6001U,0x6004U,0x6e000U,0x75000U,0x17ffc0U,0xffb20000U,UINT32_MAX}){
   uint32_t unchanged=0xaabbccdd;assert(!PioneerQ38MixedLiveProgramWord(w,address,unchanged) && unchanged==0xaabbccdd);
  }
 }
 assert(snapshots==TT_Q38_MIXED_LIVE_SAVED_WORDS && code==TT_Q38_MIXED_LIVE_CODE_WORDS);
 assert(!PioneerQ38ResidentLayerSnapshotContains(0,0x40000) && PioneerQ38MixedLiveSnapshotContains(0,0x40000)
  && !PioneerQ38ResidentLayerSnapshotContains(1,0x20000) && PioneerQ38MixedLiveSnapshotContains(1,0x20000)
  && !PioneerQ38ResidentLayerSnapshotContains(1,0x60000) && PioneerQ38MixedLiveSnapshotContains(1,0x60000)
  && !PioneerQ38ResidentLayerSnapshotContains(0,0x17f6fc) && PioneerQ38MixedLiveSnapshotContains(0,0x17f6fc));
 for(unsigned address:{0xc2000U,0xc8000U,0xd0000U,0xd6000U,0xe0000U,0xf2000U,0xfad7cU,0x100000U,0x110000U,0x118000U,0x140000U,0x156000U,0x15b000U,0x160000U,0x170000U,0x178000U,0x17f6fcU})
  assert(PioneerQ38MixedLiveSnapshotContains(0,address) && !PioneerQ38MixedLiveSnapshotContains(1,address));
 assert(!PioneerQ38MixedLiveSnapshotContains(120,0x6000) && !PioneerQ38MixedLiveCodeContains(120,0x6000));
 printf("PASS NEW QBL1+QMW1 exact code words%u/all120 complete snapshot words%u (%ubytes)/ALL10 signed sections match RAW binaries/no implicit holes/no old journal alias/no ROOT-only peer grant;GEOMETRY ONLY,NO native/journal-execution/Work authority\n",code,snapshots,snapshots*4);
}
