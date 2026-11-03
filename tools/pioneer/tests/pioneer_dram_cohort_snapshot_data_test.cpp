/* SPDX-License-Identifier: MIT */
// Pure immutable DATA comparison, not an implemented native snapshot owner.
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeDramCohortReadSweepImage.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <fstream>
#include <iterator>
#include <set>
#include <vector>
using I=BlackholeDramCohortReadSweepImage;
static std::vector<uint8_t> Read(const char* path){
 std::ifstream f(path,std::ios::binary);assert(f.good());
 return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});
}
static uint32_t Word(const std::vector<uint8_t>& v,unsigned at){
 assert(at+4<=v.size());return uint32_t(v[at])|(uint32_t(v[at+1])<<8)|(uint32_t(v[at+2])<<16)|(uint32_t(v[at+3])<<24);
}
int main(int argc,char** argv){
 assert(argc==3);const auto boot=Read(argv[1]),text=Read(argv[2]);
 assert(boot.size()==4 && text.size()==I::TextWords*4);
 assert(I::Code(0)==Word(boot,0));
 for(unsigned i=1;i<I::CodeWords;i++)assert(I::Code(i)==Word(text,(i-1)*4));
 assert(!I::Code(I::CodeWords) && !I::Code(UINT32_MAX));
 unsigned total=0,refused=0;
 for(unsigned role=0;role<8;role++){
  const bool coordinator=role==0;const unsigned count=I::SavedWords(coordinator);
  std::set<uint32_t> address;
  for(unsigned i=0;i<count;i++){
   const uint32_t a=I::Address(coordinator,i);
   assert(a!=UINT32_MAX && !(a&3) && a<0x180000 && address.insert(a).second);
   if(i<I::CodeWords)assert(a==(i?I::Entry+(i-1)*4:0x6000));
  }
  assert(address.count(0x6e000) && address.count(0x6fffc) && address.count(0x74000)
   && address.count(0x7407c) && address.count(0x74100) && address.count(0x7417c));
  for(unsigned a:{0x74200U,0x74220U,0x74240U})assert(address.count(a));
  for(unsigned a:{0x70000U,0x73ffcU,0x74080U,0x74180U,0x74204U,0x74480U,0x74880U,0xd0000U}){
   assert(!address.count(a));refused++;
  }
  if(coordinator){
   assert(count==4154 && address.count(0x74300) && address.count(0x74400) && address.count(0x7447c)
    && address.count(0x74500) && address.count(0x7487c) && !address.count(0x90000));
  }else{
   assert(count==69433 && address.count(0x90000) && address.count(0xcfffc)
    && !address.count(0x74300) && !address.count(0x74400) && !address.count(0x74500));
  }
  for(unsigned bad:{count,count+1,UINT32_MAX}){assert(I::Address(coordinator,bad)==UINT32_MAX);refused++;}
  total+=count;
 }
 assert(total==I::CohortSavedWords && total==490185);
 printf("PASS_COHORT_SNAPSHOT_DATA all8uniqueWords%u codeWordsPerTile%u boundaryRefusals%u disjointL1Extents1;NOT saved native lease/watchdog/STOP/HOLD/restore/hardware/Work authority\n",total,I::CodeWords,refused);
}
