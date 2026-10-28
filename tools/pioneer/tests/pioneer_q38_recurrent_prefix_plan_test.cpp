/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38RecurrentPrefixPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
static unsigned Word(const PioneerQ38MixedRecurrentBlockPlan& p,unsigned ordinal){
 namespace P=PioneerQ38RecurrentPrefixLayout;
 if(ordinal<128)return p.Outer()[ordinal];
 if(ordinal<P::ConvOffset/4){unsigned i=ordinal-128;return p.Recipe(i/1280)[i%1280];}
 return ordinal<P::GdnOffset/4?p.Conv()[ordinal-P::ConvOffset/4]:p.Gdn()[ordinal-P::GdnOffset/4];
}
int main(){
 namespace P=PioneerQ38RecurrentPrefixLayout;namespace R=PioneerQ38MixedRecurrentBlockRootLayout;
 auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();auto other=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build() && other->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x16005+i*131;
 std::array<uint8_t,16384> chunk;unsigned packages=0,checked=0,refused=0;
 for(unsigned bank=0;bank<7;bank++){
  auto plan=std::make_unique<PioneerQ38RecurrentPrefixPlan>();uint32_t address=0xdeadbeef;
  assert(!plan->Layer(0) && !plan->Boot(0) && plan->Bank()==7 && !plan->Workspace() && !plan->PackageAddress(0,address) && address==0xdeadbeef);
  assert(plan->Build(*canonical,bank,{1,2,3,4},sources.data()) && !plan->Build(*canonical,bank,{1,2,3,4},sources.data())
   && plan->Matches(*canonical) && !plan->Matches(*other) && plan->Bank()==bank && plan->Workspace()==canonical->Base+canonical->Workspace(bank)->offset
   && P::BootShape(plan->Boot(0)) && !R::BootShape(plan->Boot(0)) && !plan->Layer(3) && !plan->Boot(120));
  assert(plan->MatchesBinding(*canonical,bank,{1,2,3,4},sources.data()) && !plan->MatchesBinding(*other,bank,{1,2,3,4},sources.data())
   && !plan->MatchesBinding(*canonical,(bank+1)%7,{1,2,3,4},sources.data()) && !plan->MatchesBinding(*canonical,bank,{1,2,3,4},nullptr));refused+=3;
  for(auto id:{PioneerQ38GdnIdentity{2,2,3,4},{1,3,3,4},{1,2,4,4},{1,2,3,5}}){assert(!plan->MatchesBinding(*canonical,bank,id,sources.data()));refused++;}
  for(unsigned word=0;word<sources.size();word++){
   auto changed=sources;changed[word]^=1;assert(!plan->MatchesBinding(*canonical,bank,{1,2,3,4},changed.data()));refused++;
  }
  for(unsigned layer=0;layer<3;layer++){
   const auto& p=*plan->Layer(layer);assert(p.Outer()[68]==bank && p.Outer()[69]==plan->Workspace()+(layer?unsigned(R::FinalOffset):0U));
   assert(plan->PackageAddress(layer,address) && address==plan->Workspace()+P::Offset+layer*P::Stride);
   for(unsigned at=0;at<P::Bytes;at+=16384){
    unsigned bytes=std::min(16384U,unsigned(P::Bytes)-at);assert(plan->ReadPackage(layer,at,chunk.data(),bytes));
    for(unsigned i=0;i<bytes;i+=4){unsigned value=0;for(unsigned b=0;b<4;b++)value|=unsigned(chunk[i+b])<<(b*8);
     assert(value==Word(p,(at+i)/4));checked++;
    }
   }
   packages++;
   for(unsigned slot=0;slot<8;slot++)for(unsigned word:{0U,16U,32U,48U,64U,80U,1032U,1151U,1279U}){
    auto* fields=const_cast<uint32_t*>(p.Recipe(slot));fields[word]^=1;chunk.fill(0xa5);
    assert(!plan->Matches(*canonical) && !plan->ReadPackage(layer,0,chunk.data(),64));for(auto b:chunk)assert(b==0xa5);
    fields[word]^=1;assert(plan->Matches(*canonical));refused++;
   }
   for(auto* fields:{const_cast<uint32_t*>(p.Outer()),const_cast<uint32_t*>(p.Conv()),const_cast<uint32_t*>(p.Gdn())}){
    fields[0]^=1;assert(!plan->Matches(*canonical));fields[0]^=1;assert(plan->Matches(*canonical));refused++;
   }
  }
  for(unsigned worker:{0U,119U})for(unsigned i=0;i<64;i++){
   auto* boot=const_cast<uint32_t*>(plan->Boot(worker));boot[i]^=1;assert(!plan->Matches(*canonical));boot[i]^=1;assert(plan->Matches(*canonical));refused++;
  }
  for(unsigned layer:{3U,UINT32_MAX}){address=0xdeadbeef;assert(!plan->PackageAddress(layer,address) && address==0xdeadbeef);refused++;}
  for(auto request:{std::array<unsigned,2>{1,64},{0,0},{0,1},{0,16448},{unsigned(P::Bytes),64},{unsigned(P::Bytes)-64,128},{UINT32_MAX-63,64}}){
   chunk.fill(0xa5);assert(!plan->ReadPackage(0,request[0],chunk.data(),request[1]));for(auto b:chunk)assert(b==0xa5);refused++;
  }
  assert(!plan->ReadPackage(0,0,nullptr,64)
   && !plan->ReadPackage(0,0,reinterpret_cast<uint8_t*>(plan.get()),64)
   && !plan->ReadPackage(0,0,reinterpret_cast<uint8_t*>(canonical.get()),64)
   && !plan->ReadPackage(0,0,reinterpret_cast<uint8_t*>(UINTPTR_MAX-31),64));refused+=4;
  assert(!plan->PackageAddress(0,*const_cast<uint32_t*>(plan->Boot(0))) && plan->Matches(*canonical));refused++;
  std::vector<uint8_t> ledger(PioneerResidentBanks::Pages,1);assert(plan->LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));
  for(unsigned layer=0;layer<3;layer++){
   const auto* n=plan->Layer(layer)->Outer();unsigned first=n[64]*unsigned(canonical->Capacity/4096)+(n[65]-canonical->Base)/4096;
   for(unsigned invalid:{0U,2U,255U}){ledger[first]=uint8_t(invalid);assert(!plan->LearnedPagesPresent(ledger.data(),unsigned(ledger.size())));refused++;}
   ledger[first]=1;
  }
  assert(!plan->LearnedPagesPresent(nullptr,unsigned(ledger.size())) && !plan->LearnedPagesPresent(ledger.data(),unsigned(ledger.size())-1));refused+=2;
 }
 for(unsigned kind=0;kind<6;kind++){
  auto plan=std::make_unique<PioneerQ38RecurrentPrefixPlan>();PioneerQ38GdnIdentity id={1,2,3,4};unsigned bank=0;
  auto copy=sources;const uint32_t* input=copy.data();auto cold=std::make_unique<PioneerQ38CanonicalModelPlan>();
  if(kind==0)bank=7;if(kind==1)id.session=0;if(kind==2)input=nullptr;if(kind==3)copy[0]=0;if(kind==4)copy[119*16]=0;
  assert(!plan->Build(kind==5?*cold:*canonical,bank,id,input) && !plan->Layer(0) && !plan->Boot(0)
   && !plan->Build(*canonical,0,{1,2,3,4},sources.data()));refused++;
 }
 printf("PASS complete prefix immutable HEAP plan/packages%u/ALL%u serialized original metadata words/7banks/ONE canonical token0 QPE1 input->actual preceding Final/source+identity+boot+recipe+ledger+alias+range refusals%u;DATA ONLY,NO native,IO,producer or token grant\n",packages,checked,refused);
}
