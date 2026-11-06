/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Q4FpuCachedLayout.h"
#include "../PioneerQ38Q4FpuCompactData.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <set>
#include <vector>
using L=PioneerQ38Q4FpuCachedLayout;
static L::Job Job(unsigned k,unsigned groups,unsigned worker){return {{L::Magic,1,k,0x04004000,groups,786,worker,L::Peer(worker),PioneerQ38Bf16Input::Magic,L::InputBase,L::InputRoot,17,L::Output,0,0,0}};}
int main(){
 unsigned plans=0,refusals=0;std::set<unsigned> peers;
 for(unsigned w=1;w<120;w++){assert(peers.insert(L::Peer(w)).second);const unsigned xy=L::Peer(w);assert((xy&63)!=8 && (xy&63)!=9 && (xy>>6)>=2 && (xy>>6)<=11);}
 for(unsigned k:{5120U,17408U})for(unsigned groups:{2U,4U,6U,8U,16U}){
  auto job=Job(k,groups,119);assert(L::Shape(job));const unsigned blocks=k/256;
  std::vector<unsigned char> cache(L::GroupBytes(k,groups));
  for(unsigned i=0;i<cache.size();i++)cache[i]=static_cast<unsigned char>((i*53+(i/2304)*7)^0x39);
  for(unsigned pair=0;pair<groups/2;pair++)for(unsigned block=0;block<blocks;block++){
   std::array<unsigned char,4608> original,packed;
   for(unsigned row=0;row<32;row++)for(unsigned byte=0;byte<144;byte++)original[row*144+byte]=cache[((pair*2+row/16)*blocks+block)*2304+byte*16+row%16];
   for(unsigned row=0;row<32;row++)for(unsigned at:{1U,3U}){
    auto& high=original[row*144+at];if((high&0x7c)==0x7c)high^=4;
    cache[((pair*2+row/16)*blocks+block)*2304+at*16+row%16]=high;
   }
   assert(PioneerQ38Q4FpuCompactData::Prepare(original.data(),4608,packed.data(),4608));
   for(unsigned buffer=0;buffer<2;buffer++){
    std::array<unsigned char,4608> observed{};std::set<unsigned> dest,weightSources;
    unsigned bytes=0,flits=0;
    for(unsigned packet=0;packet<26;packet++){
     L::Read p={};assert(L::Packet(job,pair,block,buffer,packet,p));assert(!(p.source&63) && !(p.destination&63));
     assert(p.route==(packet<18?786U:129U));flits+=p.bytes/64;
     for(unsigned i=0;i<p.bytes;i++){
      assert(dest.insert(p.destination+i).second);
      if(packet<18){
       assert(weightSources.insert(p.source+i).second);unsigned out=0;
       if(packet<16)out=(packet%4/2)*2048+(packet/4)*512+(packet%2)*256+i;
       else out=4096+(packet-16)*256+i;
       observed[out]=cache[p.source-job.words[3]+i];
      }else assert(p.source+i==L::InputBase+block*512+(packet-18)*64+i);
     }
     for(unsigned i=0;i<64;i++)assert(!dest.count(p.destination-64+i));bytes+=p.bytes;
    }
    assert(observed==packed && bytes==5120 && flits==80 && weightSources.size()==4608);plans++;
   }
  }
 }
 auto good=Job(5120,16,1);const std::vector<std::pair<unsigned,unsigned>> bad={{0,0},{1,2},{2,6144},{3,0x03ffffc0},{3,0xc3ffffff},{3,0xffffffff},{4,0},{4,1},{4,3},{4,17},{5,129},{6,0},{6,120},{7,0},{8,0},{9,0xa0040},{10,786},{11,0},{11,0x80000000},{12,0x98000},{13,1},{14,1},{15,1}};
 for(auto item:bad){auto j=good;j.words[item.first]=item.second;L::Read p={1,2,3,4},before=p;assert(!L::Shape(j) && !L::Packet(j,0,0,0,0,p) && !memcmp(&p,&before,sizeof p));refusals++;}
 for(unsigned w=1;w<120;w++){auto j=good;j.words[6]=w;j.words[7]=L::Peer(w);assert(L::Shape(j));}
 std::array<uint32_t,256> input;std::array<uint32_t,130> output;input.fill(0x3f800000);output.fill(0xa7a7a7a7);
 assert(PioneerQ38Bf16Input::Pack(input.data(),256,output.data()+1,128) && output[0]==0xa7a7a7a7 && output[129]==0xa7a7a7a7);
 for(unsigned i=1;i<=128;i++)assert(output[i]==0x3f803f80);
 for(unsigned bits:{1U,0x007fffffU,0x7f800000U,0xff800000U,0x7fc00000U,0x7f7fffffU})for(unsigned lane:{0U,127U,255U}){
  input.fill(0x3f800000);input[lane]=bits;output.fill(0xa7a7a7a7);
  assert(!PioneerQ38Bf16Input::Pack(input.data(),256,output.data()+1,128));for(auto v:output)assert(v==0xa7a7a7a7);refusals++;
 }
 input.fill(0x3f800000);
 assert(!PioneerQ38Bf16Input::Pack(input.data(),255,output.data(),128));
 assert(!PioneerQ38Bf16Input::Pack(input.data(),256,output.data(),127));
 assert(!PioneerQ38Bf16Input::Pack(input.data(),256,input.data(),128));
 for(unsigned high=0;high<65536;high++)for(unsigned tail:{0U,32767U,32768U,65535U}){
  unsigned bits=(high<<16)|tail,value=0;
  const unsigned e=(bits>>23)&255,m=bits&0x7fffff;
  const unsigned expected=high+unsigned(tail>32768 || (tail==32768 && (high&1)));
  const bool valid=e!=255 && (e || !m) && (expected&0x7f80)!=0x7f80;
  assert(PioneerQ38Bf16Input::Lane(bits,value)==valid);if(valid)assert(value==expected);
 }
 printf("PASS cached-FPU DATA transport:%u paired16-row cache/K-block/buffer cases exact4608B original compact bytes+512B coordinator BF16;119 unique worker shapes;%u atomic refusals;262144 BF16 lane cases and chunk canaries;NO native/producer-lifetime/full-worker/hardware/quality/speed authority\n",plans,refusals);
}
