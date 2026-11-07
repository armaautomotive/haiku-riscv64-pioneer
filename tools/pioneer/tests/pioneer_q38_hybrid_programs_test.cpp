/* SPDX-License-Identifier: MIT */
// Compare every generated QGW5 code word with independent binary sections.
#define PIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC 1
#define PIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC 1
#ifndef PIONEER_Q38_HYBRID_PROGRAMS_HEADER
#error Test requires the exact freshly generated DATA header
#endif
#include PIONEER_Q38_HYBRID_PROGRAMS_HEADER
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>
namespace P=PioneerQ38HybridPrograms;namespace L=PioneerQ38HybridTokenL1;
struct Section{const char* name;unsigned address;};
static uint32_t Word(const std::vector<unsigned char>& bytes,unsigned at){
 return uint32_t(bytes[at])|uint32_t(bytes[at+1])<<8|uint32_t(bytes[at+2])<<16|uint32_t(bytes[at+3])<<24;
}
static std::vector<unsigned char> Read(const std::string& path){
 std::ifstream f(path,std::ios::binary);assert(f);
 return std::vector<unsigned char>(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());
}
int main(int argc,char** argv){
 assert(argc==3);
 const Section root[]={{"boot",0x6000},{"bf16",0x20000},{"text",0x40000},{"terminal",0x60000},{"conv",0x64000}};
 const Section peer[]={{"boot",0x6000},{"globalworker",0x20000},{"bf16producer",0x30000},{"dense",0x40000},
  {"cachedfpu",0x50000},{"ffn",0x60000},{"service",0x68000},{"affine",0x70b00},{"text",0x72000},{"gdn",0x170000}};
 unsigned long long checked=0;
 for(unsigned worker=0;worker<120;worker++){
  const Section* spec=worker?peer:root;const unsigned count=worker?10:5;unsigned ordinal=0;
  assert(P::CodeSpans(worker)==count);
  for(unsigned s=0;s<count;s++){
   const auto data=Read(std::string(argv[worker?2:1])+"/"+spec[s].name+".bin");
   const auto span=P::CodeSpan(worker,s);assert(!data.empty() && !(data.size()&3)
    && span.address==spec[s].address && span.words==data.size()/4);
   for(unsigned at=0;at<data.size();at+=4){
    uint32_t value=0;const uint32_t address=span.address+at;
    assert(L::SnapshotContains(worker,address) && P::CodeAddress(worker,ordinal++)==address
     && P::ProgramWord(worker,address,value) && value==Word(data,at));checked++;
   }
  }
  assert(ordinal==P::CodeWords(worker) && P::CodeAddress(worker,ordinal)==UINT32_MAX);
 }
 uint32_t untouched=0x12345678;
 assert(!P::ProgramWord(120,0x6000,untouched) && untouched==0x12345678
  && !P::ProgramWord(0,0x6001,untouched) && untouched==0x12345678
  && !P::ProgramWord(1,0xb8000,untouched) && untouched==0x12345678
  && !P::ProgramWord(1,0x17ffc0,untouched) && untouched==0x12345678
  && !P::CodeSpans(120) && P::CodeAddress(120,0)==UINT32_MAX);
 assert(!P::ProgramWord(0,0x6000,const_cast<uint32_t&>(P::RootBoot[0])));
 printf("PASS pinned hybrid complete emitted code/%llu words/all 120 workers/15 binary sections/relocated GDN/alias rejection; "
  "NO DEVICE, OWNER, MODEL OR SPEED\n",checked);
}
