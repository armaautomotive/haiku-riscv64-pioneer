/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38LayerChainPackage.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
#include <vector>
namespace LC=PioneerQ38LayerChainLayout;
static uint32_t Word(const std::vector<uint8_t>& bytes,unsigned at){
 return uint32_t(bytes[at])|uint32_t(bytes[at+1])<<8|uint32_t(bytes[at+2])<<16|uint32_t(bytes[at+3])<<24;
}
int main(){
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>(),foreign=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build() && foreign->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 unsigned packages=0,words=0,mutations=0,refusals=0;
 for(unsigned layer=0;layer<64;layer++){
  unsigned bank=layer%7,base=c->Base+c->Workspace(bank)->offset;
  auto p=std::make_unique<PioneerQ38LayerChainPackage>();
  assert(p->Build(*c,layer,bank,{1,2,3,4},sources.data(),bank,base) && p->Matches(*c) && !p->Matches(*foreign));
  assert(!p->Build(*c,layer,bank,{1,2,3,4},sources.data(),bank,base));
  auto r=std::make_unique<PioneerQ38MixedRecurrentBlockPlan>();auto d=std::make_unique<PioneerQ38MixedLiveBlockPlan>();
  uint32_t input=base+(layer?unsigned(LC::R::FinalOffset):0U);
  if(LC::Dense(layer))assert(d->Build(*c,layer,0,0,bank,1001+layer*16,layer+1,{1,2,3,4},sources.data(),bank,input));
  else assert(r->Build(*c,layer,0,0,bank,0,1001+layer*16,layer+1,{1,2,3,4},sources.data(),bank,input));
  std::vector<uint8_t> bytes(p->Bytes(),0xa5);
  for(unsigned at=0;at<bytes.size();at+=16384){unsigned n=std::min(16384U,unsigned(bytes.size())-at);assert(p->Read(at,bytes.data()+at,n));}
  for(unsigned at=0;at<bytes.size()/4;at++){
   uint32_t expected;
   if(at<128)expected=LC::Dense(layer)?d->Outer()[at]:r->Outer()[at];
   else if(LC::Dense(layer)){
    unsigned n=at-LC::DenseRecipes/4;expected=at<LC::DenseFrames/4?d->Recipe(n/1280)[n%1280]:d->Dense((at-LC::DenseFrames/4)/64)[(at-LC::DenseFrames/4)%64];
   }else if(at<LC::P::ConvOffset/4){unsigned n=at-LC::P::RecipesOffset/4;expected=r->Recipe(n/1280)[n%1280];}
   else expected=at<LC::P::GdnOffset/4?r->Conv()[at-LC::P::ConvOffset/4]:r->Gdn()[at-LC::P::GdnOffset/4];
   assert(Word(bytes,at*4)==expected);words++;
  }
  uint32_t address=0;assert(LC::Address(base,layer,address) && address==base+LC::Offset+layer*LC::Stride
   && LC::Frame(p->Outer(),layer,bank,base,p->Boot()));packages++;
  if(layer==0 || layer==63){
   auto mutate=[&](const uint32_t* data,unsigned count){
    for(unsigned n=0;n<count;n++){auto* word=const_cast<uint32_t*>(data)+n;*word^=1;assert(!p->Matches(*c));*word^=1;assert(p->Matches(*c));mutations++;}
   };
   mutate(p->Outer(),128);mutate(p->Boot(),64);
   std::array<uint8_t,64> untouched;untouched.fill(0xa5);
   for(unsigned bad=0;bad<8;bad++){
    assert(!p->Read(bad==0?1:bad==1?p->Bytes():bad==2?p->Bytes()+64:0,
     bad==3?nullptr:bad==4?reinterpret_cast<uint8_t*>(p.get()):bad==5?reinterpret_cast<uint8_t*>(c.get()):untouched.data(),
     bad==6?0:bad==7?1:64));
    for(auto b:untouched)assert(b==0xa5);refusals++;
   }
   assert(p->Matches(*c));
  }
 }
 for(unsigned bad=0;bad<5;bad++){
  auto p=std::make_unique<PioneerQ38LayerChainPackage>();
  assert(!p->Build(*c,bad==0?64:0,bad==1?7:0,{1,2,3,4},bad==2?nullptr:sources.data(),bad==3?7:0,bad==4?1:c->Base));
  assert(!p->Outer() && !p->Boot() && !p->Bytes());refusals++;
 }
 printf("PASS QCH1 canonical package DATA/all64 layers/rotating ALL7banks/48recurrent+16dense/serializedwords%u/exact independent plans/mutations%u/refusals%u/no-alias+no-partial-output;NO model-auth/producer/state/native/device/Work or TPS grant/packages%u\n",words,mutations,refusals,packages);
}
