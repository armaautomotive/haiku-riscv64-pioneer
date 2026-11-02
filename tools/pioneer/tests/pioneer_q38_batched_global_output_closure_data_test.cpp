/* SPDX-License-Identifier: MIT */
// Synthetic closed-receipt DATA,NOT an executed worker or ROOT history.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#include "../PioneerQ38BatchedGlobalOutputClosure.h"
#include "../PioneerQ38GlobalOutputPlan.h"
#include "../PioneerQ38GlobalHeadPlan.h"
#include "../PioneerQ38GlobalChainPackage.h"
#include "../PioneerQ38MixedRecurrentProjectionPlan.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <memory>
namespace Old=PioneerQ38GlobalOutputClosure;
namespace New=PioneerQ38BatchedGlobalOutputClosure;
namespace G=PioneerQ38SelectedGlobalWorker;
struct Receipt {
 std::array<uint32_t,128> h={};std::array<uint32_t,64> root={},boot={},d={},job={},partial={},proof={};
 std::array<uint32_t,16> status={};std::array<uint32_t,8> physical={};unsigned nonce=1001;
 bool Valid(bool batched)const{
  const auto f=batched?New::Validate:Old::Validate;
  return f(h.data(),root.data(),boot.data(),d.data(),nonce,job.data(),partial.data(),proof.data(),status.data(),physical.data());
 }
 void Counts(bool batched){
  const unsigned groups=job[23];
  for(unsigned i=0;i<8;i++){
   const unsigned delta=i==1?groups*1060+162:i==0 || i==2 || i==3 || i==6?groups*(batched?10:20)+2:0;
   proof[i]=0xfffffff8U;physical[i]=proof[i]+delta;proof[8+i]=partial[16+i]=physical[i];
  }
 }
};
static Receipt Make(const PioneerQ38CanonicalModelPlan& c,const uint32_t* sources,unsigned worker,unsigned position,unsigned role){
 Receipt x;const unsigned workspace=c.Base+c.Workspace(0)->offset;
 auto package=std::make_unique<PioneerQ38GlobalChainPackage>();
 assert(package->Build(c,0,0,{1,2,3,4},sources,0,workspace+0x600000));
 for(unsigned i=0;i<64;i++)x.root[i]=package->Boot()[i];
 auto output=std::make_unique<PioneerQ38GlobalOutputPlan>();assert(output->Build(c,position-1,{1,2,3,4},sources));
 const unsigned wave=worker<=21?8:7;
 assert(output->WorkerBoot(worker,x.boot.data()) && output->Job(wave,worker,x.nonce-1,x.job.data()));
 auto head=std::make_unique<PioneerQ38GlobalHeadPlan>();assert(head->Build(c,0,x.root.data()));
 for(unsigned i=0;i<128;i++)x.h[i]=head->Data()[i];
 auto plan=std::make_unique<PioneerQ38MixedRecurrentProjectionPlan>();
 const unsigned offset=role==10?0:role==11?1:2;
 assert(plan->Build(c,0,role,position,0,0,1001+position*1024+offset,position*64+1,{1,2,3,4},sources));
 for(unsigned i=0;i<64;i++)x.d[i]=plan->Frame()[i];
 const unsigned status[]={x.nonce,x.nonce,0,G::Magic,0,0,64,position-1};
 for(unsigned i=0;i<8;i++)x.status[i]=status[i];
 const unsigned groups=x.job[23];
 const unsigned partial[]={G::PartialMagic,x.nonce,64,position-1,worker,wave,x.job[28],groups*16,x.job[28],0,1,x.job[7],248320,5120,14,1};
 for(unsigned i=0;i<16;i++)x.partial[i]=partial[i];
 x.proof[16]=groups*320;x.proof[17]=x.nonce;x.proof[18]=worker;x.proof[19]=0x32504651;
 const unsigned command[]={x.job[22]+(groups*20-1)*3392,0,x.job[24],0x90000,0,x.boot[4],0,0,3392,0};
 for(unsigned i=0;i<10;i++)x.partial[24+i]=x.proof[20+i]=command[i];
 x.proof[30]=x.boot[16];x.proof[31]=x.boot[4];
 for(unsigned i=0;i<16;i++)x.partial[34+i]=x.proof[32+i]=x.boot[16+i];
 for(unsigned i=0;i<8;i++)x.partial[50+i]=x.boot[8+i];
 x.Counts(true);return x;
}
int main(){
 auto c=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(c->Build());
 std::array<uint32_t,1920> sources;for(unsigned i=0;i<sources.size();i++)sources[i]=0x76540000+i+1;
 unsigned positive=0,refused=0;
 for(unsigned position:{1U,2U,8191U})for(unsigned worker=1;worker<=119;worker++)for(unsigned role:{10U,11U,5U}){
  auto x=Make(*c,sources.data(),worker,position,role);const bool allowed=worker>3 || role==10;
  assert(x.Valid(true)==allowed && !x.Valid(false));
  x.Counts(false);assert(!x.Valid(true) && x.Valid(false)==allowed);x.Counts(true);
  if(!allowed){refused++;continue;}
  positive++;
  if(position!=1 || role!=10 || (worker!=1 && worker!=21 && worker!=22 && worker!=119))continue;
  auto mutate=[&](auto& words,unsigned first,unsigned count){
   for(unsigned i=first;i<first+count;i++)for(unsigned bit=0;bit<32;bit++){
    words[i]^=1U<<bit;assert(!x.Valid(true));words[i]^=1U<<bit;refused++;
   }
  };
  mutate(x.root,0,64);mutate(x.boot,0,64);mutate(x.job,0,64);mutate(x.proof,0,64);
  mutate(x.status,0,16);mutate(x.physical,0,8);mutate(x.partial,0,8);mutate(x.partial,10,54);mutate(x.h,16,32);
  mutate(x.h,64+((worker<=21?8:7)*119+worker-1)%7*8,8);
  for(unsigned factor:{0U,5U,9U,11U,15U,19U,20U,21U}){
   for(unsigned i:{0U,2U,3U,6U}){
    x.physical[i]=x.proof[8+i]=x.partial[16+i]=x.proof[i]+x.job[23]*factor+2;
   }
   assert(!x.Valid(true));x.Counts(true);refused++;
  }
  assert(x.Valid(true));
 }
 printf("PASS SYNTHETIC batched global-output closure family%08x/%u canonical DATA positives/%u wrong-role,bit+count refusals/old20 ALWAYS rejected by new10 and new10 ALWAYS rejected by old20/modulo32-wrap/final command+flits unchanged;NOT worker,ROOT,token,owner,native,physical or TPS proof\n",G::Magic,positive,refused);
}
