/* SPDX-License-Identifier: MIT */
// The new bridge feeds actual unchanged QMW1 instructions, not a mocked
// successful job/nonce/clock. Numeric endpoints remain synthetic here.
#define PIONEER_Q38_MIXED_WORKER_MACHINE_ONLY
#include "pioneer_q38_mixed_worker_test.cpp"
#include "../PioneerQ38MixedRecurrentDispatch.h"
namespace Dispatch=PioneerQ38MixedRecurrentDispatch;
static void Install(MixedPeer& p,const uint32_t* j){
 assert(p.pc==p.waitPC || p.pc==p.waitPC+4);
 assert(j[1]==p.sequence+1 && M::JobShape(j,p.boot.data(),p.sequence,0));
 for(unsigned i=0;i<64;i++)p.memory[M::Job+i*4]=j[i];
 p.memory[M::Doorbell]=j[1];p.sequence=j[1];
}
static bool Pair(MixedPeer& a,MixedPeer& b){
 a.peer=&b;b.peer=&a;
 while(!a.GdnFinished(a.sequence) || !b.GdnFinished(b.sequence)){
  for(auto* p:{&a,&b})if(!p->GdnFinished(p->sequence)){
   p->StepBlock();if(p->GdnFaulted())return false;
  }
 }
 return true;
}
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RunPersistentGdnPair; // this test intentionally uses unequal per-peer nonces
 unsigned pairs=0,refusals=0,stack=0;
 for(unsigned head:{0U,1U,19U,31U,47U}){
  auto a=std::make_unique<MixedPeer>(argv[1],head*2+1),b=std::make_unique<MixedPeer>(argv[1],head*2+2);
  // Unequal actual histories MUST NOT be erased/aligned by writing nonces.
  // Rendezvous uses the shared phase/identity,not equal private job numbers.
  for(auto* p:{a.get(),b.get()}){
   assert(p->BootMixed() && p->MatrixMixed());
   unsigned extra=head==1?unsigned(p==a.get())*2:unsigned(p==b.get());
   for(unsigned job=0;job<extra;job++){FreshInputs(*p);assert(p->MatrixMixed());}
   unsigned previous=p->sequence;
   p->StageConv(previous+1,1002);
   uint32_t j[64];assert(Dispatch::Conv(p->boot.data(),p->table.data(),previous,j));
   // Table source/epoch binding is mandatory independently of shape.
   for(unsigned word:{16U,31U,32U,39U}){
    auto bad=p->table;bad[word]^=1;uint32_t sentinel[64];for(auto& v:sentinel)v=0xdecafbad;
    assert(!Dispatch::Conv(p->boot.data(),bad.data(),previous,sentinel));
    for(auto v:sentinel)assert(v==0xdecafbad);refusals++;
   }
   for(unsigned word=0;word<64;word++){
    auto bad=p->boot;bad[word]^=1;uint32_t sentinel[64];for(auto& v:sentinel)v=0xdecafbad;
    assert(!Dispatch::Conv(bad.data(),p->table.data(),previous,sentinel));
    for(auto v:sentinel)assert(v==0xdecafbad);refusals++;
   }
   for(unsigned word=0;word<384;word++){
    auto bad=p->table;bad[word]^=1;uint32_t sentinel[64];for(auto& v:sentinel)v=0xdecafbad;
    assert(!Dispatch::Conv(p->boot.data(),bad.data(),previous,sentinel));
    for(auto v:sentinel)assert(v==0xdecafbad);refusals++;
   }
   assert(!Dispatch::Conv(nullptr,p->table.data(),previous,j) && !Dispatch::Conv(p->boot.data(),nullptr,previous,j)
    && !Dispatch::Conv(p->boot.data(),p->table.data(),previous,nullptr)
    && !Dispatch::Conv(p->boot.data(),p->table.data(),0x7ffffffe,j)
    && !Dispatch::Conv(p->boot.data(),p->table.data(),previous,p->boot.data())
    && !Dispatch::Conv(p->boot.data(),p->table.data(),previous,p->table.data()));refusals+=6;
   Install(*p,j);assert(p->FinishMixed(p->sequence));p->VerifyConv();
   previous=p->sequence;p->StageGdn(previous+1,1003);
   assert(Dispatch::Gdn(p->boot.data(),p->input.data(),p->state.data(),p->gated.data(),previous,j));
   for(unsigned word:{16U,31U,32U,39U}){
    auto bad=p->gated;bad[word]^=1;uint32_t sentinel[64];for(auto& v:sentinel)v=0xdecafbad;
    assert(!Dispatch::Gdn(p->boot.data(),p->input.data(),p->state.data(),bad.data(),previous,sentinel));
    for(auto v:sentinel)assert(v==0xdecafbad);refusals++;
   }
   for(unsigned word=0;word<64;word++){
    auto bad=p->boot;bad[word]^=1;uint32_t sentinel[64];for(auto& v:sentinel)v=0xdecafbad;
    assert(!Dispatch::Gdn(bad.data(),p->input.data(),p->state.data(),p->gated.data(),previous,sentinel));
    for(auto v:sentinel)assert(v==0xdecafbad);refusals++;
   }
   for(unsigned which=0;which<3;which++)for(unsigned word=0;word<64;word++){
    auto input=p->input,state=p->state,gated=p->gated;auto& bad=which==0?input:which==1?state:gated;bad[word]^=1;
    uint32_t sentinel[64];for(auto& v:sentinel)v=0xdecafbad;
    assert(!Dispatch::Gdn(p->boot.data(),input.data(),state.data(),gated.data(),previous,sentinel));
    for(auto v:sentinel)assert(v==0xdecafbad);refusals++;
   }
   for(auto* alias:{p->boot.data(),p->input.data(),p->state.data(),p->gated.data()}){
    assert(!Dispatch::Gdn(p->boot.data(),p->input.data(),p->state.data(),p->gated.data(),previous,alias));refusals++;
   }
   assert(!Dispatch::Gdn(nullptr,p->input.data(),p->state.data(),p->gated.data(),previous,j)
    && !Dispatch::Gdn(p->boot.data(),nullptr,p->state.data(),p->gated.data(),previous,j)
    && !Dispatch::Gdn(p->boot.data(),p->input.data(),nullptr,p->gated.data(),previous,j)
    && !Dispatch::Gdn(p->boot.data(),p->input.data(),p->state.data(),nullptr,previous,j)
    && !Dispatch::Gdn(p->boot.data(),p->input.data(),p->state.data(),p->gated.data(),previous,nullptr)
    && !Dispatch::Gdn(p->boot.data(),p->input.data(),p->state.data(),p->gated.data(),0x7ffffffe,j));refusals+=6;
   Install(*p,j);
  }
  assert(a->sequence!=b->sequence && Pair(*a,*b));a->VerifyGdn(a->sequence);b->VerifyGdn(b->sequence);
  for(auto* p:{a.get(),b.get()}){
   // Return to real matrix arithmetic after GDN without rebuilding state.
   assert(p->MatrixMixed());p->StopMixed();stack=std::max(stack,0x70000-p->minStack);
  }
  printf("PASS dispatch ACTUAL mixed Conv/GDN unequal closed histories/head%u/independent arithmetic/real matrix continuation+STOP\n",head);pairs++;
 }
 assert(stack<=8192);
 printf("PASS dispatch pairs%u/atomic source+identity refusals%u/stack%u;host DATA bridge+actual worker instructions,NOT ROOT coordinator/native/physical/fullmodel/TPS\n",pairs,refusals,stack);
}
