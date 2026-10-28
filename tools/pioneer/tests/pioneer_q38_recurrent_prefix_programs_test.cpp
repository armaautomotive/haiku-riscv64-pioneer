/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38RecurrentPrefixPrograms.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
int main(int argc,char** argv){
 assert(argc==3);const char* rootNames[]={"boot","text","conv"};
 const char* peerNames[]={"boot","mixed","dense","ffn","service","affine","text","gdn"};
 std::array<std::vector<uint8_t>,11> files;
 for(unsigned i=0;i<11;i++){
  std::ifstream file(std::string(argv[i<3?1:2])+"/"+(i<3?rootNames[i]:peerNames[i-3])+".bin",std::ios::binary);assert(file);
  files[i]=std::vector<uint8_t>(std::istreambuf_iterator<char>(file),{});assert(!files[i].empty());
 }
 unsigned checked=0,saved=0,refused=0;
 for(unsigned w=0;w<120;w++){
  unsigned words=0;
  for(unsigned n=0;n<PioneerQ38RecurrentPrefixCodeSpans(w);n++){
   const auto s=PioneerQ38RecurrentPrefixCodeSpan(w,n);const auto& raw=files[(w?3:0)+n];assert(raw.size()==s.words*4);
   for(unsigned i=0;i<s.words;i++){
    uint32_t expected=0,actual=0xa5a5a5a5;for(unsigned b=0;b<4;b++)expected|=uint32_t(raw[i*4+b])<<(b*8);
    assert(PioneerQ38RecurrentPrefixCodeAddress(w,words+i)==s.address+i*4 && PioneerQ38RecurrentPrefixSnapshotContains(w,s.address+i*4)
     && PioneerQ38RecurrentPrefixCodeContains(w,s.address+i*4) && PioneerQ38RecurrentPrefixProgramWord(w,s.address+i*4,actual) && actual==expected);checked++;
   }
   uint32_t actual=0xa5a5a5a5;assert(!PioneerQ38RecurrentPrefixProgramWord(w,s.address+1,actual) && actual==0xa5a5a5a5);refused++;
   words+=s.words;
  }
  assert(words==PioneerQ38RecurrentPrefixCodeWords(w) && PioneerQ38RecurrentPrefixCodeAddress(w,words)==UINT32_MAX);
  unsigned total=0;
  for(unsigned n=0;n<PioneerQ38RecurrentPrefixSnapshotSpans(w);n++){
   const auto s=PioneerQ38RecurrentPrefixSnapshotSpan(w,n);
   assert(s.words && PioneerQ38RecurrentPrefixSnapshotAddress(w,total)==s.address
    && PioneerQ38RecurrentPrefixSnapshotAddress(w,total+s.words-1)==s.address+(s.words-1)*4);total+=s.words;
  }
  assert(total==PioneerQ38RecurrentPrefixSnapshotWords(w) && PioneerQ38RecurrentPrefixSnapshotAddress(w,total)==UINT32_MAX);
  saved+=total;
  for(unsigned address:{0U,0x6004U,0x6dffcU,0x17ffc0U,UINT32_MAX}){
   uint32_t value=0xa5a5a5a5;assert(!PioneerQ38RecurrentPrefixProgramWord(w,address,value) && value==0xa5a5a5a5);refused++;
  }
 }
 for(unsigned w:{120U,UINT32_MAX}){uint32_t value=0xa5a5a5a5;
  assert(!PioneerQ38RecurrentPrefixSnapshotSpans(w) && !PioneerQ38RecurrentPrefixCodeSpans(w)
   && !PioneerQ38RecurrentPrefixProgramWord(w,0x6000,value) && value==0xa5a5a5a5);refused++;
 }
 assert(checked==TT_Q38_RECURRENT_PREFIX_CODE_WORDS && saved==TT_Q38_RECURRENT_PREFIX_SAVED_WORDS);
 printf("PASS QPX1 distinct program DATA/ALL11 artifacts+ALL120 participants/%u codewords BIT-EXACT/%u saved geometry words/padding+unaligned+invalid-worker refusals%u;NO native journal,installation or launch authority\n",checked,saved,refused);
}
