/* SPDX-License-Identifier: MIT */
// Real canonical recipes, fake transport. No accelerator access.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38TokenMetadata.h"
#include "../PioneerQ38TokenRecipeBatch.h"
#include "../PioneerQ38TokenStatePlan.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>

namespace B=PioneerQ38TokenRecipeBatch;
namespace C=B::G;
namespace M=PioneerQ38TokenMetadata;

struct Fixture {
 unsigned from,to,first,count,layer,position,token;
 bool gddr;
 unsigned reads=0,writes=0,local=0,transforms=0,completed=0,inflight=0,done=0,failRead=0,failWrite=0;
 std::vector<uint32_t> source,out;
 std::array<uint32_t,B::ScratchBytes/4> scratch={};
 Fixture(unsigned f,unsigned t,unsigned s,unsigned n,unsigned l,unsigned p,unsigned k,bool g)
  :from(f),to(t),first(s),count(n),layer(l),position(p),token(k),gddr(g),
   source(n*B::Bytes/4),out(n*B::Bytes/4,0x5a5a5a5a){}
 bool Read(unsigned addr,unsigned bytes){
  assert(!inflight && addr==from+done && bytes && !(bytes%5120) && bytes<=15360 && done+bytes<=source.size()*4);
  reads++;
  if(reads==failRead)return false;
  std::copy_n(source.begin()+done/4,bytes/4,scratch.begin());inflight=bytes;completed=0;return true;
 }
 bool Transform(unsigned at,unsigned slot){
  assert(inflight && at==completed && at+5120<=inflight && slot==first+(done+at)/5120);
  transforms++;completed+=5120;
  return M::Recipe(scratch.data()+at/4,layer,slot,position,token);
 }
 bool Publish(unsigned addr,unsigned bytes,bool isGddr){
  assert(bytes==inflight && completed==bytes && isGddr==gddr && addr==to+done);
  if(isGddr)writes++;else local++;
  if(writes+local==failWrite)return false;
  std::copy_n(scratch.begin(),bytes/4,out.begin()+done/4);done+=bytes;inflight=0;return true;
 }
};

int main(){
 (void)&M::Outer;(void)&M::Conv;(void)&M::Gdn;(void)&M::Dense;
 setvbuf(stdout,nullptr,_IOLBF,0);
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540001+i;
 const PioneerQ38GdnIdentity id={1,2,3,4};
 unsigned tested=0,recipes=0,refused=0;
 for(unsigned bank=0;bank<7;bank++){
  unsigned oldPackets=0,newPackets=0;
  const unsigned workspace=canonical->Base+canonical->Workspace(bank)->offset;
  for(unsigned layer=0;layer<64;layer++){
   auto package=std::make_unique<PioneerQ38GlobalChainPackage>();
   assert(package->Build(*canonical,layer,bank,id,sources.data(),bank,workspace+0x600000));
   uint32_t root=0;assert(C::Address(workspace,layer,root));
   const auto* outer=package->Outer();assert(outer);
   const bool dense=C::Dense(layer);
   for(unsigned part=0;part<(dense?2U:1U);part++){
    const unsigned first=part?4:0,count=!dense?8:part?3:4;
    const unsigned offset=!dense?unsigned(C::P::RecipesOffset):part?unsigned(C::DenseFfn):unsigned(C::DenseRecipes);
    const unsigned output=!dense?outer[57]:part?outer[68]:unsigned(C::D::Recipes);
    const bool gddr=!dense || part;
    std::vector<uint32_t> source(count*B::Bytes/4);
    for(unsigned i=0;i<count;){
     const unsigned n=B::Take(count-i),at=i*B::Bytes;
     assert(package->Read(offset+at,reinterpret_cast<uint8_t*>(source.data()+at/4),n*B::Bytes));
     i+=n;
    }
    for(unsigned pos:{0U,31U,8191U}){
     Fixture next(root+offset,output,first,count,layer,pos,248319-pos,gddr);
     next.source=source;auto expected=source;
     for(unsigned i=0;i<count;i++)assert(M::Recipe(expected.data()+i*B::Bytes/4,layer,first+i,pos,next.token));
     assert(B::Run(next,next.from,next.to,first,count,gddr) && next.source==source && next.out==expected);
     const unsigned batches=count==8?3:count==4?2:1;
     assert(next.reads==batches && next.writes==(gddr?batches:0) && next.local==(gddr?0:batches)
      && next.transforms==count && next.done==count*B::Bytes && !next.inflight);
     if(pos==31){oldPackets+=count*(gddr?2:1);newPackets+=batches*(gddr?2:1);}
     if(bank==0 && layer<4 && pos==31){
      for(unsigned i=0;i<count;i++){
       Fixture bad(next.from,next.to,first,count,layer,pos,next.token,gddr);
       bad.source=source;bad.source[i*B::Bytes/4]=0; // invalid canonical recipe magic
       assert(!B::Run(bad,bad.from,bad.to,first,count,gddr) && bad.done==(i/3)*3*B::Bytes);
       assert(std::equal(bad.out.begin(),bad.out.begin()+bad.done/4,expected.begin()));
       assert(std::all_of(bad.out.begin()+bad.done/4,bad.out.end(),[](uint32_t w){return w==0x5a5a5a5a;}));refused++;
      }
      for(bool readFailure:{false,true}){
       Fixture bad(next.from,next.to,first,count,layer,pos,next.token,gddr);bad.source=source;
       if(readFailure)bad.failRead=1;else bad.failWrite=1;
       assert(!B::Run(bad,bad.from,bad.to,first,count,gddr) && bad.reads==1 && !bad.done);refused++;
      }
     }
     tested++;recipes+=count;
    }
   }
  }
  assert(oldPackets==928 && newPackets==352 && oldPackets-newPackets==576);
  printf("RECIPE_BATCH canonical bank%u all64 layers verified\n",bank);
 }
 Fixture bad(0x04000000,0x04100000,0,8,0,31,0,true);
 assert(!B::Run(bad,bad.from+1,bad.to,0,8,true)
  && !B::Run(bad,bad.from,bad.from+5120,0,8,true)
  && !B::Run(bad,0xfffff000,bad.to,0,8,true)
  && !B::Run(bad,bad.from,bad.to,0,0,true)
  && !B::Run(bad,bad.from,bad.to,0,4,false)
  && !B::Run(bad,bad.from,bad.to,3,3,true) && !bad.reads && !bad.writes);
 printf("PASS recipe grouping: %u batches of canonical groups, %u exact transforms, %u refusals, "
  "928 -> 352 requests per position (same bytes); fake transport, no timing\n",tested,recipes,refused+6);
}
