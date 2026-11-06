/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "../PioneerQ38Q4FpuPairIssueCompute.h"
struct Trace {
 std::vector<uint64_t> words;
 void Op(unsigned v){words.push_back(v);}
 void Config(unsigned i,unsigned v){words.push_back((UINT64_C(1)<<63)|(uint64_t(i)<<32)|v);}
 void Drain(){words.push_back(UINT64_C(1)<<62);}
};
int main(){
 using Old=PioneerQ38Q4FpuCompactStreamCompute;using New=PioneerQ38Q4FpuPairIssueCompute;
 unsigned cases=0;
 Trace oldBegin,newBegin;Old::Begin(oldBegin);New::Begin(newBegin);assert(oldBegin.words==newBegin.words);
 for(unsigned buffer=0;buffer<2;buffer++){
  Trace oldMeta,newMeta;assert(Old::LoadMetadata(oldMeta,buffer) && New::LoadMetadata(newMeta,buffer));
  assert(oldMeta.words==newMeta.words);
  for(unsigned group=0;group<8;group++){
   Trace a,b;assert(Old::Group(a,buffer,group) && New::Group(b,buffer,group));
   assert(a.words==b.words);cases++;
  }
 }
 Trace refused;
 for(unsigned b:{2U,3U,~0U})assert(!New::LoadMetadata(refused,b) && !New::Group(refused,b,0));
 for(unsigned g:{8U,9U,~0U})assert(!New::Group(refused,0,g));
 assert(refused.words.empty());
 printf("PASS_PAIRED_ISSUE words/config/drains_byte_equal_groups%u init_metadata_equal invalid_zeroIO;NOT_TIMING_OR_NATIVE_AUTHORITY\n",cases);
}
