/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38HeldSnapshotBatch.h"
#include <cassert>
#include <cstdio>

namespace B = PioneerQ38HeldSnapshotBatch;
namespace L = PioneerQ38TokenRedistributedL1;

static uint32_t Word(unsigned worker,uint32_t address)
{
 return ((address & 0x1ff)==0?UINT32_MAX:address^uint32_t(worker<<24));
}

struct FakeIO {
 unsigned scopeCalls=0,loadCalls=0,reads=0;
 unsigned failScope=0;
 bool failLoad=false;
 bool Scope(unsigned){return ++scopeCalls!=failScope;}
 bool Load(unsigned worker,const B::Batch& batch,uint32_t* words){
  loadCalls++;
  for(unsigned i=0;i<batch.words;i++){
   if(failLoad && i==1)return false;
   reads++;words[i]=Word(worker,batch.address+4*i);
  }
  return true;
 }
};

static void CheckGeometry()
{
 unsigned batches=0,scalar=0;
 FakeIO io;
 for(unsigned worker=0;worker<L::Workers;worker++){
  const unsigned prior=io.reads;
  for(unsigned pos=0;pos<L::SnapshotWords(worker);){
   B::Batch batch{};
   assert(B::Next(worker,pos,batch));
   assert(batch.ordinal==pos && batch.words<=B::MaxWords && batch.words>0);
   assert(batch.address==L::SnapshotAddress(worker,pos));
   B::Window window=B::Window::Scalar;
   assert(B::EndOfWindow(batch.address,window)>=batch.address+4*batch.words && window==batch.window);
   if(window==B::Window::Scalar){assert(batch.words==1);scalar++;}
   uint32_t captured[B::MaxWords];
   assert(B::Read(io,worker,batch,captured));
   for(unsigned i=0;i<batch.words;i++){
    const uint32_t address=batch.address+4*i;
    assert(L::SnapshotContains(worker,address) && address==L::SnapshotAddress(worker,pos+i));
    assert(captured[i]==Word(worker,address));
   }
   batches++;pos+=batch.words;
  }
  assert(io.reads-prior==L::SnapshotWords(worker));
 }
 assert(io.reads==L::SavedWords && scalar==L::Workers*9);
 assert(io.loadCalls==batches && io.scopeCalls==2*batches);
 printf("PASS exact %u held snapshot words, mapped groups %u, legacy scalar exceptions %u; both passes still required\n",
  io.reads,batches-scalar,scalar);
}

static void CheckFaults()
{
 constexpr unsigned worker=1;
 B::Batch good{};
 assert(B::Next(worker,1,good) && good.words==B::MaxWords && good.window==B::Window::Low);
 uint32_t destination[B::MaxWords];
 for(auto& word:destination)word=0x86549711;
 auto attempt=[&](unsigned index,const B::Batch& batch,FakeIO& io,unsigned scopes,unsigned loads){
  assert(!B::Read(io,index,batch,destination));
  assert(io.scopeCalls==scopes && io.loadCalls==loads);
  for(uint32_t word:destination)assert(word==0x86549711);
 };
 B::Batch invalid=good;
 FakeIO noIo;
 attempt(L::Workers,good,noIo,0,0);
 invalid.ordinal=L::SnapshotWords(worker);attempt(worker,invalid,noIo,0,0);
 invalid=good;invalid.address+=4;attempt(worker,invalid,noIo,0,0);
 invalid=good;invalid.address=0x22000;attempt(worker,invalid,noIo,0,0);
 invalid=good;invalid.words++;attempt(worker,invalid,noIo,0,0);
 invalid=good;invalid.window=B::Window::Panel;attempt(worker,invalid,noIo,0,0);
 assert(!B::Read(noIo,worker,good,nullptr) && !noIo.scopeCalls && !noIo.loadCalls);
 FakeIO pre;pre.failScope=1;attempt(worker,good,pre,1,0);
 FakeIO load;load.failLoad=true;attempt(worker,good,load,1,1);assert(load.reads==1);
 FakeIO post;post.failScope=2;attempt(worker,good,post,2,1);assert(post.reads==B::MaxWords);
 puts("PASS invalid span/window/bounds and pre/read/post failure never publish prepared original words");
}

int main()
{
 CheckGeometry();CheckFaults();
}
