/* SPDX-License-Identifier: MIT */
// Immutable DATA access only, not native initialization or model execution.
#ifndef PIONEER_Q38_PUSHED_PROGRAMS_HEADER
#define PIONEER_Q38_PUSHED_PROGRAMS_HEADER "../PioneerQ38TokenPushedPrograms.h"
#endif
#include PIONEER_Q38_PUSHED_PROGRAMS_HEADER
#ifdef PIONEER_Q38_TOKEN_PROGRAMS_H
#error The distinct G3 package must not silently select the legacy package
#endif
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
namespace P=PioneerQ38TokenPushedPrograms;
namespace S=PioneerQ38TokenPushedL1;
struct Code {const char* file;unsigned address;};
static std::vector<unsigned char> Bytes(const char* directory,const char* name){
 std::ifstream file(std::string(directory)+"/"+name+".bin",std::ios::binary);assert(file);
 return {std::istreambuf_iterator<char>(file),{}};
}
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 assert(P::Family==S::Family && P::Family==0x33505451 && P::Version==1 && P::CancelBoundary==1);
 const Code root[]={{"boot",0x6000},{"text",0x40000},{"terminal",0x60000},{"conv",0x64000}};
 const Code peer[]={{"boot",0x6000},{"globalworker",0x20000},{"precise",0x30000},
  {"dense",0x40000},{"ffn",0x60000},{"service",0x68000},{"affine",0x70b00},{"text",0x72000},{"gdn",0xb8000}};
 unsigned allWords=0,refusals=0;
 for(unsigned worker=0;worker<120;worker++){
  const unsigned count=worker?9:4;const Code* sections=worker?peer:root;
  assert(P::CodeSpans(worker)==count);unsigned words=0,last=0;
  for(unsigned section=0;section<count;section++){
   const auto span=P::CodeSpan(worker,section);const auto bytes=Bytes(worker?argv[2]:argv[1],sections[section].file);
   assert(span.address==sections[section].address && span.words && span.address>=last && bytes.size()==span.words*4);
   for(unsigned i=0;i<span.words;i++){
    const unsigned address=span.address+i*4,offset=i*4;uint32_t word=0;
    const uint32_t expected=bytes[offset]|uint32_t(bytes[offset+1])<<8|uint32_t(bytes[offset+2])<<16|uint32_t(bytes[offset+3])<<24;
    assert(P::CodeAddress(worker,words++)==address && S::SnapshotContains(worker,address)
     && P::ProgramWord(worker,address,word) && word==expected);
   }
   last=span.address+span.words*4;
   uint32_t unchanged=0xdeadbeef;
   assert(!P::ProgramWord(worker,last,unchanged) && unchanged==0xdeadbeef);refusals++;
  }
  assert(words==P::CodeWords(worker) && P::CodeAddress(worker,words)==UINT32_MAX
   && P::CodeAddress(worker,UINT32_MAX)==UINT32_MAX);
  allWords+=words;
  const auto invalid=P::CodeSpan(worker,count);assert(invalid.address==UINT32_MAX && !invalid.words);
  for(unsigned address:{0U,0x6001U,0x6004U,0x6e000U,0x74000U,0x83c00U,0x118000U,0x17ffc0U,0xffb20000U,UINT32_MAX}){
   uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(worker,address,unchanged) && unchanged==0xdeadbeef);refusals++;
  }
 }
 assert(allWords==3048868 && P::CodeWords(0)==40905 && P::CodeWords(1)==25277);
 for(unsigned worker:{120U,UINT32_MAX}){
  uint32_t unchanged=0xdeadbeef;const auto span=P::CodeSpan(worker,0);
  assert(!P::CodeSpans(worker) && !P::CodeWords(worker) && P::CodeAddress(worker,0)==UINT32_MAX
   && span.address==UINT32_MAX && !span.words && !P::ProgramWord(worker,0x6000,unchanged) && unchanged==0xdeadbeef);refusals++;
 }
 const uint32_t* arrays[]={P::RootBoot,P::RootText,P::RootTerminal,P::RootAux,P::PeerBoot,P::PeerFront,
  P::PeerPrecise,P::PeerDense,P::PeerFfn,P::PeerService,P::PeerAffine,P::PeerText,P::PeerGdn};
 for(const auto* array:arrays){
  const uint32_t before=*array;assert(!P::ProgramWord(0,0x6000,*const_cast<uint32_t*>(array)) && *array==before);refusals++;
 }
 printf("PASS distinct QTP3 immutable programs/all13 sections/ALL120 code%u words byte-exact to authenticated artifacts/covered by proposed journal/%u gap+runtime+invalid+immutable-output refusals;DATA ONLY,not native/restore/model/physical/Work/TPS qualification\n",allWords,refusals);
}
