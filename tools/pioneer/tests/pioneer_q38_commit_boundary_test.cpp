/* SPDX-License-Identifier: MIT */
// Actual production function fragments in a SYNTHETIC private frame with
// MOCK HeadSeal/Transfer/Packet. NOT native ownership or device execution.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>
#include "../PioneerQ38TokenCommitBoundaryLayout.h"
namespace L=PioneerQ38GlobalChainLayout;namespace R=L::R;
namespace S=PioneerQ38TokenStateLayout;namespace F=PioneerQ38GlobalFeedbackLayout;
namespace CB=PioneerQ38TokenCommitBoundaryLayout;
enum {Bank=0,Workspace=1,Completed=3,Position=16,Committed=18,Slot=19};
struct Private {uint32_t boot[64]={},outer[128]={},state[64]={},phase=4;};
static_assert(sizeof(Private)==1028,"same private-frame size,NOT a genuine runtime owner");
struct Span {unsigned bank=0,address=0;std::vector<uint32_t> destination;};
struct Layer {std::array<uint32_t,64> table={};std::array<Span,7> spans;std::vector<uint32_t> history;};
struct Numeric {unsigned address,words;};
static const Numeric Numbers[]={
 {R::Original,5120},{R::OriginalCopy,5120},{R::Input,5120},{R::InputShadow,5120},
 {R::Native,2560},{R::NativeShadow,2560},{R::Scales,32},{R::ScaleShadow,32},
 {F::Local,5120},{F::Shadow,5120}};
struct Command {
 unsigned kind,route,address,bytes;
 bool operator==(const Command& b)const{return kind==b.kind && route==b.route && address==b.address && bytes==b.bytes;}
};
enum class Hook {None,Numeric,Mirror,Head,Readback,Unknown};
struct Fixture {
 Private owner;std::vector<uint32_t> l1;std::array<Layer,64> layers;
 std::vector<Command> commands;unsigned active=64,packets=0,transfers=0,inputScans=0,feedbackScans=0;
 Hook hook=Hook::None;unsigned injectAt=1,numeric=0;bool injected=false,headValid=true,faulted=false;
 Fixture():l1(0x180000/4,0){
  owner.state[Bank]=3;owner.state[Workspace]=0x08000000;owner.state[Completed]=64;
  for(unsigned pair=0;pair<10;pair+=2){
   assert(Numbers[pair].words==Numbers[pair+1].words);
   for(unsigned i=0;i<Numbers[pair].words;i++)
    l1[Numbers[pair].address/4+i]=l1[Numbers[pair+1].address/4+i]=0x3f000000U+(pair*17+i)%65536;
  }
  for(unsigned layer=0;layer<64;layer++){
   if(L::Dense(layer))continue;
   auto& item=layers[layer];auto& d=item.table;uint32_t history=0;
   assert(PioneerQ38RecurrentHistoryLayout::Address(owner.state[Workspace],layer,history));
   const unsigned head[]={S::Magic,1,layer,owner.state[Bank],owner.state[Workspace],history,
                          PioneerQ38RecurrentHistoryLayout::Bytes,7};
   std::copy(head,head+8,d.begin());unsigned first=0;
   for(unsigned span=0;span<7;span++){
    auto* p=d.data()+8+span*8;const unsigned rows=span<6?1472:1408;
    const unsigned fields[]={span,0x40000000+layer*0x20000,first,rows,12,rows*12,
                              (rows*12+4095)&~4095U,PioneerQ38ProjectionRoute(span)};
    std::copy(fields,fields+8,p);first+=rows;
    item.spans[span].bank=span;item.spans[span].address=p[1];
    item.spans[span].destination.assign(p[5]/4,0xa5a50000U+span);
   }
   assert(first==10240 && S::Shape(d.data(),layer,owner.state[Bank],owner.state[Workspace]));
   item.history.resize(d[6]/4);
   for(unsigned i=0;i<item.history.size();i++)item.history[i]=0x3e000000U+(layer*257+i)%65536;
  }
 }
};
static Fixture* current=nullptr;
struct Fault {};
[[noreturn]] static void scalar_fault_park(){assert(current);current->faulted=true;throw Fault{};}
static Private* Owner(){assert(current && !current->faulted);return &current->owner;}
static volatile uint32_t* Words(unsigned address){
 assert(current && !current->faulted && !(address&3) && address<current->l1.size()*4);
 return current->l1.data()+address/4;
}
static void HeadSeal(const uint32_t*){
 const auto* p=Owner();if(!current->headValid || p->state[Completed]!=64
  || (p->phase!=4 && p->phase!=6) || p->state[Committed]!=(p->phase==6?1U:0U)
  || p->state[Slot]!=(p->phase==6?1U:0U))scalar_fault_park();
}
// Compile the ACTUAL unchanged numeric equality/finite checks,not a second
// handwritten approximation. Wrappers count calls,NOT cycles or performance.
#define InputSeal ActualInputSeal
#include PIONEER_Q38_COMMIT_INPUT_FRAGMENT
#undef InputSeal
static void InputSeal(){current->inputScans++;ActualInputSeal();}
#define FeedbackSeal ActualFeedbackSeal
#include PIONEER_Q38_COMMIT_FEEDBACK_FRAGMENT
#undef FeedbackSeal
static void FeedbackSeal(const uint32_t* h){current->feedbackScans++;ActualFeedbackSeal(h);}
static void Transfer(unsigned address,unsigned bytes,bool write){
 const uint32_t h[128]={};HeadSeal(h);assert(!write && bytes==256);
 for(unsigned layer=0;layer<64;layer++){
  uint32_t package=0;if(!L::Address(Owner()->state[Workspace],layer,package))continue;
  if(address!=package+S::Offset)continue;
  assert(!L::Dense(layer));current->active=layer;current->transfers++;
  current->commands.push_back({0,Owner()->state[Bank],address,bytes});
  const auto& table=current->layers[layer].table;
  for(unsigned i=0;i<64;i++)Words(R::Scratch)[i]=table[i];return;
 }
 scalar_fault_park();
}
static void Packet(const uint32_t* h,unsigned route,unsigned address,unsigned bytes,bool write){
 HeadSeal(h);assert(current->active<64 && bytes && bytes<=16384-(address&31) && !((bytes|address)&3));
 auto& layer=current->layers[current->active];current->packets++;
 current->commands.push_back({write?2U:1U,route,address,bytes});bool found=false;
 if(!write && route==PioneerQ38ProjectionRoute(Owner()->state[Bank])
  && address>=layer.table[5] && address-layer.table[5]<=layer.table[6]-bytes){
  const unsigned first=(address-layer.table[5])/4;
  for(unsigned i=0;i<bytes/4;i++)Words(R::Scratch)[i]=layer.history[first+i];found=true;
 }else for(auto& span:layer.spans){
  if(route!=PioneerQ38ProjectionRoute(span.bank) || address<span.address
   || bytes>span.destination.size()*4 || address-span.address>span.destination.size()*4-bytes)continue;
  const unsigned first=(address-span.address)/4;
  for(unsigned i=0;i<bytes/4;i++){
   if(write)span.destination[first+i]=Words(R::Scratch)[i];
   else Words(R::Scratch)[i]=span.destination[first+i];
  }
  found=true;break;
 }
 assert(found);
 if(current->packets==current->injectAt && current->hook!=Hook::None){
  current->injected=true;
  switch(current->hook){
   case Hook::Numeric:*Words(current->numeric)^=1;break;
   case Hook::Mirror:*Words(S::Local)^=1;break;
   case Hook::Head:current->headValid=false;break;
   case Hook::Readback:assert(!write && current->packets%3==0);*Words(R::Scratch)^=1;break;
   case Hook::Unknown:scalar_fault_park();
   case Hook::None:assert(false);
  }
 }
}
#define __attribute__(...)
namespace Before {
#include PIONEER_Q38_COMMIT_STATE_FRAGMENT
}
#define PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE 1
namespace After {
#include PIONEER_Q38_COMMIT_STATE_FRAGMENT
}
#undef PIONEER_Q38_TOKEN_COMMIT_BOUNDARY_SEALS_IMAGE
#undef __attribute__
static bool Run(Fixture& f,bool after,bool feedback){
 assert(!f.faulted);current=&f;uint32_t h[128]={};
 try{if(after)After::CommitState(h,feedback);else Before::CommitState(h,feedback);}
 catch(const Fault&){current=nullptr;assert(f.faulted);return false;}
 current=nullptr;return true;
}
static void NotCommitted(const Fixture& f){
 assert(f.faulted && f.owner.phase==4 && !f.owner.state[Committed] && !f.owner.state[Slot]);
}
int main(){
 unsigned valid=0,refused=0;
 for(const auto& n:Numbers){assert(!CB::NumericDisjoint(n.address,n.words*4));}
 assert(!CB::Apart(0xfffffff0U,32,0x100,4) && !CB::Apart(0x100,4,0xfffffff0U,32));
 for(bool feedback:{false,true}){
  auto before=std::make_unique<Fixture>(),after=std::make_unique<Fixture>();
  assert(Run(*before,false,feedback) && Run(*after,true,feedback));valid+=2;
  assert(before->commands==after->commands && before->l1==after->l1);
  assert(after->transfers==48 && after->packets==2016 && after->owner.phase==6
   && after->owner.state[Committed]==1 && after->owner.state[Slot]==1);
  assert(before->inputScans==(feedback?2786U:1393U) && after->inputScans==(feedback?4U:2U));
  assert(before->feedbackScans==(feedback?1393U:0U) && after->feedbackScans==(feedback?2U:0U));
  for(unsigned layer=0;layer<64;layer++)if(!L::Dense(layer))for(unsigned span=0;span<7;span++){
   const auto& d=after->layers[layer].table;const auto& result=after->layers[layer].spans[span].destination;
   const auto& expected=after->layers[layer].history;const unsigned first=d[8+span*8+2]*3;
   assert(std::equal(result.begin(),result.end(),expected.begin()+first));
   assert(result==before->layers[layer].spans[span].destination);
  }
  printf("OBSERVED MOCK CommitState feedback%u identical2064commands+5898240copiedbytes+slotflip1;"
         "actualInputSealCalls old%u new%u actualFeedbackSealCalls old%u new%u;NOT cycles_or_TPS\n",
         unsigned(feedback),before->inputScans,after->inputScans,before->feedbackScans,after->feedbackScans);
  for(unsigned family=0;family<(feedback?10U:8U);family++)for(bool last:{false,true}){
   const unsigned address=Numbers[family].address+(last?Numbers[family].words-1:0)*4;
   for(bool optimized:{false,true}){
    auto f=std::make_unique<Fixture>();f->l1[address/4]^=1;
    assert(!Run(*f,optimized,feedback));NotCommitted(*f);refused++;
    assert(f->packets==0 && f->transfers==(optimized?0U:1U));
   }
   for(unsigned at:{1U,2016U}){
    auto f=std::make_unique<Fixture>();f->hook=Hook::Numeric;f->numeric=address;f->injectAt=at;
    assert(!Run(*f,true,feedback));NotCommitted(*f);refused++;
    assert(f->injected && f->packets==2016 && f->transfers==48); // detected BEFORE flip,not earlier
   }
  }
  for(bool optimized:{false,true})for(Hook hook:{Hook::Mirror,Hook::Head,Hook::Readback,Hook::Unknown}){
   auto f=std::make_unique<Fixture>();f->hook=hook;f->injectAt=hook==Hook::Readback?3:1;
   assert(!Run(*f,optimized,feedback));NotCommitted(*f);refused++;
   assert(f->injected && f->transfers==1 && f->packets<4);
  }
 }
 printf("PASS actual StateSeal+CommitState+InputSeal+FeedbackSeal fragments/%u valid/%u fault cases/"
        "unchanged copy bytes+commands/readback+metadata failures before flip/initial numeric failure before IO/"
        "mid-copy numeric corruption detected at transaction EXIT before slot/frontier publication;"
        "SYNTHETIC private frame+MOCK HeadSeal/Transfer/Packet,NOT original model/native lifecycle/physical/Work/TPS\n",valid,refused);
}
