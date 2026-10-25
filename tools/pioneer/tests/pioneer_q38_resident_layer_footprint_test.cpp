/* SPDX-License-Identifier: MIT */
// Pure DATA geometry;NO mapping,source,initialized or native capability.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentLayerFootprint.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputFootprint.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
static unsigned Bytes(const char* root,const char* section){std::ifstream in(std::string(root)+"/"+section+".bin",std::ios::binary|std::ios::ate);assert(in);return unsigned(in.tellg());}
int main(int argc,char** argv){
 assert(argc==3);unsigned saved=0,code=0;
 const char* names[2][7]={{"boot","conv","matrix","text","cohort","norm","ffn"},{"boot","ffn","service","affine","text","gdn",nullptr}};
 for(unsigned w=0;w<120;w++){
  assert(PioneerQ38ResidentLayerSnapshotBase(w)==saved);
  std::vector<uint8_t> snapshot(0x180000/4,0),program(0x180000/4,0);unsigned ordinal=0,words=0;
  for(unsigned i=0;i<PioneerQ38ResidentLayerSpanCount(w);i++){
   auto s=PioneerQ38ResidentLayerSnapshotSpan(w,i);assert(s.words && s.address<0x180000 && s.words<=(0x180000-s.address)/4);
   for(unsigned n=0;n<s.words;n++,ordinal++){
    unsigned a=s.address+n*4;assert(!snapshot[a/4]++ && PioneerQ38ResidentLayerSnapshotAddress(w,ordinal)==a
     && PioneerQ38ResidentLayerSnapshotContains(w,a) && !PioneerQ38ResidentLayerSnapshotContains(w,a+1));
   }
  }
  assert(ordinal==PioneerQ38ResidentLayerSnapshotWords(w) && PioneerQ38ResidentLayerSnapshotAddress(w,ordinal)==UINT32_MAX);saved+=ordinal;
  for(unsigned i=0;i<PioneerQ38ResidentLayerCodeSpanCount(w);i++){
   auto s=PioneerQ38ResidentLayerCodeSpan(w,i);assert(s.words*4==Bytes(argv[w?2:1],names[!!w][i]));words+=s.words;
   for(unsigned n=0;n<s.words;n++){unsigned a=s.address+n*4;assert(snapshot[a/4] && !program[a/4]++ && PioneerQ38ResidentLayerProgramImmutable(w,a));}
  }
  assert(words==PioneerQ38ResidentLayerCodeWords(w));code+=words;
  for(unsigned a=0;a<0x180000;a+=4){
   assert(bool(snapshot[a/4])==PioneerQ38ResidentLayerSnapshotContains(w,a));
   assert(bool(program[a/4])==PioneerQ38ResidentLayerProgramImmutable(w,a));
   // The OLD owner remains unchanged and may NOT cover the NEW ROOT ROM.
   if(w)assert(bool(snapshot[a/4])==PioneerQ38ResidentOutputSnapshotContains(w,a));
  }
  for(unsigned n=0;n<PioneerQ38ResidentOutputSnapshotWords(w);n++)assert(PioneerQ38ResidentLayerSnapshotContains(w,PioneerQ38ResidentOutputSnapshotAddress(w,n)));
 }
 assert(saved==11000888 && code==2270704);
 for(unsigned a:{0x14e000U,0x152000U,0x158000U,0x15c000U,0x17dffcU})assert(PioneerQ38ResidentLayerSnapshotContains(0,a) && !PioneerQ38ResidentOutputSnapshotContains(0,a));
 for(unsigned a:{0x14e000U,0x152000U,0x157ffcU,0x15951cU,0x17dffcU,0x17e000U,0x17ffc0U})assert(!PioneerQ38ResidentLayerProgramImmutable(0,a));
 for(unsigned w:{120U,121U,~0U}){
  assert(!PioneerQ38ResidentLayerSpanCount(w) && !PioneerQ38ResidentLayerCodeSpanCount(w) && !PioneerQ38ResidentLayerSnapshotWords(w)
   && !PioneerQ38ResidentLayerCodeWords(w) && PioneerQ38ResidentLayerSnapshotBase(w)==UINT32_MAX
   && PioneerQ38ResidentLayerSnapshotAddress(w,0)==UINT32_MAX && !PioneerQ38ResidentLayerSnapshotContains(w,0x6000)
   && !PioneerQ38ResidentLayerProgramImmutable(w,0x6000));
 }
 puts("PASS NEW layer snapshot DATA:ALL120/11000888arbitrary original words/44003552B/2270704exact codewords/ALL7ROOT+ALL6peer emitted extents/exhaustive L1 coverage+no overlap/boundaries+invalid workers/old68 geometry unchanged;NO allocation,mapping,write,restore,source,initialized,owner or launch grant");
}
