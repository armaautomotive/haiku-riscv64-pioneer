/* SPDX-License-Identifier: MIT */
// Coverage of proposed DATA geometry, NOT execution of a native journal.
#define PIONEER_Q38_TOKEN_PRECISE_PROGRAMS 1
#include "../PioneerQ38TokenFootprint.h"
#include "../PioneerQ38TokenPushedFootprint.h"
#include "../PioneerQ38PushedWorkerLayout.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
namespace S=PioneerQ38TokenPushedL1;
namespace Old=PioneerQ38TokenL1;
namespace G=PioneerQ38PushedWorkerLayout;
struct Code {const char* file;unsigned address;};
static std::vector<unsigned char> Bytes(const char* directory,const char* name){
 std::ifstream file(std::string(directory)+"/"+name+".bin",std::ios::binary);assert(file);
 return {std::istreambuf_iterator<char>(file),{}};
}
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 const Code root[]={{"boot",0x6000},{"text",0x40000},{"terminal",0x60000},{"conv",0x64000}};
 const Code peer[]={{"boot",0x6000},{"globalworker",0x20000},{"precise",0x30000},
  {"dense",0x40000},{"ffn",0x60000},{"service",0x68000},{"affine",0x70b00},{"text",0x72000},{"gdn",0xb8000}};
 unsigned saved=0,codeWords=0,oldMissing=0,padding=0;
 assert(S::Family==0x33505451 && S::Workers==120 && Old::Precise==1
  && Old::PeerSavedWords==109577 && S::PeerSavedWords-Old::PeerSavedWords==1024);
 for(unsigned worker=0;worker<S::Workers;worker++){
  unsigned ordinal=0,last=0;
  assert(S::SnapshotBase(worker)==saved);
  for(unsigned i=0;i<S::SnapshotSpans(worker);i++){
   const auto span=S::SnapshotSpan(worker,i);
   assert(span.words && !(span.address&3) && span.address>=last && span.address+span.words*4<=0x180000);
   for(unsigned at=0;at<span.words;at++){
    const unsigned address=span.address+at*4;
    assert(S::SnapshotAddress(worker,ordinal++)==address && S::SnapshotContains(worker,address)
     && !S::SnapshotContains(worker,address+1));
   }
   last=span.address+span.words*4;
  }
  assert(ordinal==S::SnapshotWords(worker) && S::SnapshotAddress(worker,ordinal)==UINT32_MAX);
  saved+=ordinal;
  const Code* sections=worker?peer:root;const unsigned count=worker?9:4;
  for(unsigned i=0;i<count;i++){
   const auto& section=sections[i];const auto bytes=Bytes(worker?argv[2]:argv[1],section.file);
   assert(!bytes.empty() && !(bytes.size()&3) && bytes.size()<=0x180000-section.address);
   for(unsigned at=0;at<bytes.size();at+=4){
    const unsigned address=section.address+at;assert(S::SnapshotContains(worker,address));codeWords++;
    if(!Old::SnapshotContains(worker,address)){
     assert(worker && i==1 && address>=0x22000 && address<0x223a8);oldMissing++;
    }
   }
   if(worker && i==1){
    assert(bytes.size()==9128 && section.address+bytes.size()==0x223a8);
    for(unsigned address=0x223a8;address<0x23000;address+=4){assert(S::SnapshotContains(worker,address));padding++;}
    assert(!S::SnapshotContains(worker,0x23000));
   }
  }
  if(worker){
   for(unsigned i=0;i<256;i+=4)assert(S::SnapshotContains(worker,G::PushBuffer+i));
   for(unsigned i=0;i<64;i++)assert(S::SnapshotContains(worker,G::Boot+i*4));
   assert(S::SnapshotContains(worker,0x3fffc) && S::SnapshotContains(worker,0x6e000)
    && S::SnapshotContains(worker,0x6fffc) && S::SnapshotContains(worker,0x17ffdc));
  }else{
   for(unsigned w=1;w<=119;w++)for(unsigned i=0;i<256;i+=4)assert(S::SnapshotContains(0,G::Slot(w)+i));
   assert(S::SnapshotContains(0,0x60000) && S::SnapshotContains(0,0x60004));
  }
 }
 assert(saved==S::SavedWords && oldMissing==119*234 && padding==119*790);
 for(unsigned worker:{120U,UINT32_MAX}){
  const auto span=S::SnapshotSpan(worker,0);
  assert(!S::SnapshotSpans(worker) && !S::SnapshotWords(worker) && S::SnapshotBase(worker)==UINT32_MAX
   && S::SnapshotAddress(worker,0)==UINT32_MAX && !S::SnapshotContains(worker,0x6000)
   && span.address==UINT32_MAX && !span.words);
 }
 for(unsigned worker:{0U,1U,119U}){
  const auto span=S::SnapshotSpan(worker,S::SnapshotSpans(worker));assert(span.address==UINT32_MAX && !span.words);
  for(unsigned address:{0U,0x6004U,0x17ffe0U,0xffb20000U,UINT32_MAX})assert(!S::SnapshotContains(worker,address));
  // ROOT has a gap before its stack; peers save the whole service/stack window.
  assert(S::SnapshotContains(worker,0x6dffc)==(worker!=0));
 }
 printf("PASS proposed QTP3 footprint/all120 code%u words/saved%u words=%u bytes/old QTP2 misses%u code words/new front padding%u words/receipt+push buffer+boot+stack covered;GEOMETRY ONLY,not native journal/restore/model/physical/Work/TPS qualification\n",
  codeWords,saved,S::SavedBytes,oldMissing,padding);
}
