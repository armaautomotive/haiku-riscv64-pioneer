/* SPDX-License-Identifier: MIT */
// Exact immutable program and complete saved-word GEOMETRY only. No native
// owner,physical IO,model numerics or disposal capability is exercised.
#ifndef PIONEER_Q38_TOKEN_PROGRAMS_HEADER
#define PIONEER_Q38_TOKEN_PROGRAMS_HEADER "../PioneerQ38TokenPrograms.h"
#endif
#include PIONEER_Q38_TOKEN_PROGRAMS_HEADER
#include "../PioneerQ38RecurrentPrefixFootprint.h"
#include "../PioneerQ38TokenLayout.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
namespace S=PioneerQ38TokenL1;namespace P=PioneerQ38TokenPrograms;
static std::vector<unsigned char> Bytes(const char* directory,const char* name){
 std::ifstream f(std::string(directory)+"/"+name+".bin",std::ios::binary);assert(f.good());return {std::istreambuf_iterator<char>(f),{}};
}
int main(int argc,char** argv){
 assert(argc==4 && unsigned(std::stoul(argv[3]))==P::CancelBoundary);setvbuf(stdout,nullptr,_IOLBF,0);
 const char* root[]={"boot","text","terminal","conv"};
 const char* peer[]={"boot","globalworker",
#ifdef PIONEER_Q38_TOKEN_PRECISE_PROGRAMS
  "precise",
#endif
  "dense","ffn","service","affine","text","gdn"};
 unsigned snapshots=0,code=0,refusals=0;
 for(unsigned worker=0;worker<120;worker++){
  unsigned words=0,last=0;
  for(unsigned i=0;i<S::SnapshotSpans(worker);i++){
   const auto span=S::SnapshotSpan(worker,i);assert(span.words && !(span.address&3) && span.address>=last && span.address+span.words*4<=0x180000);
   for(unsigned at=0;at<span.words;at++){
    assert(S::SnapshotAddress(worker,words)==span.address+at*4 && S::SnapshotContains(worker,span.address+at*4));words++;
   }
   last=span.address+span.words*4;
  }
  assert(words==S::SnapshotWords(worker) && S::SnapshotBase(worker)==snapshots && S::SnapshotAddress(worker,words)==UINT32_MAX);snapshots+=words;
  unsigned count=0;
  for(unsigned i=0;i<P::CodeSpans(worker);i++){
   const auto span=P::CodeSpan(worker,i);auto bytes=Bytes(worker?argv[2]:argv[1],worker?peer[i]:root[i]);
   assert(bytes.size()==span.words*4);
   for(unsigned at=0;at<span.words;at++){
    unsigned address=span.address+at*4,value=0;
    assert(P::CodeAddress(worker,count)==address && S::SnapshotContains(worker,address) && P::ProgramWord(worker,address,value));
    unsigned offset=at*4,wanted=bytes[offset]|uint32_t(bytes[offset+1])<<8|uint32_t(bytes[offset+2])<<16|uint32_t(bytes[offset+3])<<24;
    assert(value==wanted);count++;
   }
  }
  assert(count==P::CodeWords(worker) && P::CodeAddress(worker,count)==UINT32_MAX);code+=count;
  for(unsigned address:{0U,0x6001U,0x6004U,0x6e000U,0x75000U,0x17ffc0U,0xffb20000U,UINT32_MAX}){
   uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(worker,address,unchanged) && unchanged==0xdeadbeef);refusals++;
  }
 }
 assert(snapshots==S::SavedWords && code==P::CodeWords(0)+119*P::CodeWords(1));
#ifdef PIONEER_Q38_TOKEN_PRECISE_PROGRAMS
 assert(P::Family==0x32505451 && S::Precise==1 && P::CancelBoundary==1
  && P::CodeWords(0)==40017 && P::CodeWords(1)==24776 && S::SavedWords==13356602);
 for(unsigned worker=1;worker<120;worker++){
  assert(S::SnapshotContains(worker,0x30000) && S::SnapshotContains(worker,0x3fffc));
  uint32_t word=0xdeadbeef;assert(P::ProgramWord(worker,0x30000,word));
  assert(!P::ProgramWord(worker,0x3fffc,word)); // saved padding is NOT executable program DATA
 }
#else
 assert(P::Family==0x31505451 && S::Precise==0 && P::CodeWords(0)==unsigned(P::CancelBoundary?37647:37160) && P::CodeWords(1)==22463);
#endif
 for(unsigned address:{0x60000U,0x60004U})assert(S::SnapshotContains(0,address) && !PioneerQ38RecurrentPrefixSnapshotContains(0,address));
 for(unsigned address:{unsigned(PioneerQ38TokenLayout::SessionLocal),unsigned(PioneerQ38TokenLayout::RecordLocal),
  unsigned(PioneerQ38TokenLayout::TerminalLocal),unsigned(PioneerQ38TokenLayout::CancelLocal),0x6e000U,0x6fffcU,0x17f7fcU})
  assert(S::SnapshotContains(0,address));
 for(unsigned worker:{120U,UINT32_MAX}){
  uint32_t unchanged=0xdeadbeef;
  assert(!S::SnapshotSpans(worker) && !S::SnapshotWords(worker) && S::SnapshotBase(worker)==UINT32_MAX
   && S::SnapshotAddress(worker,0)==UINT32_MAX && !S::SnapshotContains(worker,0x6000)
   && !P::CodeSpans(worker) && !P::CodeWords(worker) && P::CodeAddress(worker,0)==UINT32_MAX
   && !P::ProgramWord(worker,0x6000,unchanged) && unchanged==0xdeadbeef);refusals++;
 }
 uint32_t* rootAlias=const_cast<uint32_t*>(P::RootText);const uint32_t before=*rootAlias;
 assert(!P::ProgramWord(0,0x6000,*rootAlias) && *rootAlias==before);refusals++;
 printf("PASS FP32 token program DATA/cancel%u/ALL%u section bytes/ALL120 exact code%u words/snapshot%u words=%ubytes/terminal60000+60004 NEW vs old prefix/runtime+padding covered/%u refusals/no implicit padding or immutable-output alias;NOT native/journal-execution/physical/model/Work/TPS authority\n",P::CancelBoundary,4+P::CodeSpans(1),code,snapshots,S::SavedBytes,refusals);
}
