/* SPDX-License-Identifier: MIT */
// Exact code/geometry DATA, not a physical save/restore or execution test.
#include "fast_decode_native_role_defines.inc"
#include "../PioneerQ38TokenSelectedPrograms.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
namespace P=PioneerQ38TokenPrograms;
namespace S=PioneerQ38TokenL1;
struct Code {const uint32_t* words;unsigned bytes,address;const char* file;};
int main(int argc,char** argv){
 assert(argc==2 && PioneerQ38FullOffloadFirmwareSelection==12 && P::RootSections==5 && P::FastDecodeProfile==1);
 assert(S::SavedBytes==53915328 && S::RootSavedWords==317313 && S::RootAuxWords==8566
  && S::FastDecode==1 && S::PreparedQ4==0 && S::PeerSavedWords==110601);
 const Code root[]={{P::RootBoot,sizeof(P::RootBoot),0x6000,"boot"},{P::RootText,sizeof(P::RootText),0x40000,"text"},
  {P::RootTerminal,sizeof(P::RootTerminal),0x60000,"terminal"},{P::RootAux,sizeof(P::RootAux),0x64000,"conv"},
  {P::RootFastDecode,sizeof(P::RootFastDecode),0x6c000,"fast_decode"}};
 const Code peer[]={{P::PeerBoot,sizeof(P::PeerBoot),0x6000,nullptr},{P::PeerFront,sizeof(P::PeerFront),0x20000,nullptr},
  {P::PeerPrecise,sizeof(P::PeerPrecise),0x30000,nullptr},{P::PeerDense,sizeof(P::PeerDense),0x40000,nullptr},
  {P::PeerFfn,sizeof(P::PeerFfn),0x60000,nullptr},{P::PeerService,sizeof(P::PeerService),0x68000,nullptr},
  {P::PeerFetch,sizeof(P::PeerFetch),0x70000,nullptr},{P::PeerAffine,sizeof(P::PeerAffine),0x70b00,nullptr},
  {P::PeerText,sizeof(P::PeerText),0x72000,nullptr},{P::PeerGdn,sizeof(P::PeerGdn),0xb8000,nullptr}};
 for(const auto& part:root){
  std::ifstream file(std::string(argv[1])+"/"+part.file+".bin",std::ios::binary);assert(file);
  const std::vector<unsigned char> raw{std::istreambuf_iterator<char>(file),{}};
  assert(raw.size()==part.bytes);
  for(unsigned i=0;i<raw.size();i++)assert(raw[i]==uint8_t(part.words[i/4]>>((i&3)*8)));
 }
 unsigned saved=0,code=0,refused=0;
 for(unsigned worker=0;worker<120;worker++){
  unsigned ordinal=0,last=0;
  for(unsigned i=0;i<S::SnapshotSpans(worker);i++){
   const auto span=S::SnapshotSpan(worker,i);assert(span.address>=last && span.words);
   for(unsigned j=0;j<span.words;j++){
    const auto address=span.address+j*4;
    assert(S::SnapshotAddress(worker,ordinal++)==address && S::SnapshotContains(worker,address));
   }
   last=span.address+span.words*4;
  }
  assert(ordinal==S::SnapshotWords(worker) && S::SnapshotBase(worker)==saved);
  assert(S::SnapshotAddress(worker,ordinal)==UINT32_MAX);saved+=ordinal;
  const Code* parts=worker?peer:root;ordinal=0;
  assert(P::CodeSpans(worker)==(worker?10U:5U));
  for(unsigned i=0;i<P::CodeSpans(worker);i++){
   const auto span=P::CodeSpan(worker,i);const auto& part=parts[i];
   assert(span.address==part.address && span.words*4==part.bytes);
   for(unsigned j=0;j<span.words;j++){
    uint32_t word=0;const auto address=span.address+j*4;
    assert(P::CodeAddress(worker,ordinal++)==address && S::SnapshotContains(worker,address)
     && P::ProgramWord(worker,address,word) && word==part.words[j]);
   }
   uint32_t unchanged=0xdeadbeef;
   assert(!P::ProgramWord(worker,part.address,const_cast<uint32_t&>(part.words[0])));
   assert(!P::ProgramWord(worker,span.address+span.words*4,unchanged) && unchanged==0xdeadbeef);refused+=2;
  }
  assert(ordinal==P::CodeWords(worker) && P::CodeAddress(worker,ordinal)==UINT32_MAX);code+=ordinal;
 }
 assert(saved==S::SavedWords);
 for(unsigned address=0x6c000;address<0x6c5d8;address+=4)assert(S::SnapshotContains(0,address));
 for(unsigned address:{0x6c5d8U,0x6dffcU,0x6c001U})assert(!S::SnapshotContains(0,address));
 for(unsigned address:{0x6e000U,0x6fffcU,0x75710U,0x757fcU}){
  uint32_t unchanged=0xdeadbeef;
  assert(S::SnapshotContains(0,address) && !P::ProgramWord(0,address,unchanged) && unchanged==0xdeadbeef);
 }
 for(unsigned worker:{120U,UINT32_MAX}){
  uint32_t unchanged=0xdeadbeef;
  assert(!P::CodeSpans(worker) && !P::CodeWords(worker) && !S::SnapshotSpans(worker)
   && !S::SnapshotContains(worker,0x6c000) && !P::ProgramWord(worker,0x6c000,unchanged) && unchanged==0xdeadbeef);
 }
 printf("PASS fast decode native DATA: all120 workers/%u snapshot words/%u code words/%u aliases+endpoints refused/exact374-word extension/no padding or stack code grant;NOT hardware lifecycle or TPS\n",saved,code,refused);
}
