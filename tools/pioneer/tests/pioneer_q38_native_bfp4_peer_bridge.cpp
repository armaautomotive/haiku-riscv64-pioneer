/* SPDX-License-Identifier: MIT */
// Host-only BFP4 tensor and queued-read endpoints, never a device owner.
#define PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE 1
#include "pioneer_q38_native_bfp4_peer_bridge.h"
#define PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT 1
#define PIONEER_BFP4_WORKER_LOADER_TEST_ONLY 1
// BFP8 includes the same test source with a different IO layout. Give this
// helper a distinct C++ type so linking both bridges cannot merge methods.
#define IO Q38IsolatedBfp4LoaderIO
#include "pioneer_q38_bfp4_worker_loader_test.cpp"
#undef IO
#define PIONEER_NATIVE_BFP4_MACHINE_ONLY 1
#include "pioneer_q38_native_bfp4_band_numeric_test.cpp"
#include <algorithm>
#include <cassert>
#include <memory>

struct Bfp4PeerMachine:Band4Machine {
 const unsigned bands;
 Bfp4PeerMachine(unsigned k,unsigned count):Band4Machine(k,L::Weight,L::Input,L::WeightStride,L::InputStride,L::Header,L::Output),bands(count){assert(bands && bands<=8);}
 void Op(unsigned op){
  if(op==0x10180000){assert(clears<bands && unpacks[0]==clears*tiles && unpacks[1]==unpacks[0] && matrixOps==clears*tiles*8);
   clears++;NativeBfpMachine::Op(op);
  }else Band4Machine::Op(op);
 }
};
struct Q38NativeBfp4PeerBridge::Impl {
 Q38NativePeerTestBytes bytes;unsigned worker;W::Job job;Bfp4PeerMachine native;Q38IsolatedBfp4LoaderIO io;
 uint32_t published[8]={},initial[8]={},commands[10]={};bool pending=false,awaiting=false;unsigned ack=0;
 static unsigned Index(unsigned i){const unsigned indices[]={2,3,4,5,1,8,4,10};return indices[i];}
 static unsigned Offset(unsigned i){const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return offsets[i];}
 Impl(const uint32_t in[16],unsigned w,const Q38NativePeerTestBytes& b)
  :bytes(b),worker(w),job{},native(in[2],in[4]),io(job,0,&native.l1){
  for(unsigned i=0;i<16;i++)job.words[i]=in[i];assert(W::Shape(job) && job.words[6]==worker);
  io.job=job;io.weights.resize(size_t(job.words[4])*L::Tiles(job.words[2])*L::WeightPayload);
  io.root.resize(job.words[2]*2);io.autoBand=true;
  assert(bytes.context && bytes.word && bytes.noc && bytes.events && bytes.read);
  for(unsigned at=0;at<io.weights.size();){const unsigned n=std::min<unsigned>(16384,unsigned(io.weights.size()-at));
   assert(bytes.read(bytes.context,job.words[5],job.words[3]+at,io.weights.data()+at,n));at+=n;}
  for(unsigned i=0;i<io.root.size()/4;i++){const uint32_t word=bytes.word(bytes.context,0,job.words[9]+i*4);
   for(unsigned j=0;j<4;j++)io.root[i*4+j]=uint8_t(word>>(j*8));}
  for(unsigned i=0;i<8;i++)initial[i]=published[i]=io.counters[Index(i)]=bytes.noc(bytes.context,worker,Offset(i));
  for(unsigned i=0;i<10;i++)commands[i]=io.command[i]=bytes.noc(bytes.context,worker,i*4);
 }
 bool Loaded()const{const unsigned perBand=L::Tiles(job.words[2])+(job.words[2]*2+16383)/16384;
  return io.completions>=perBand && !(io.completions%perBand) && io.pending.empty();}
 void Publish(const std::vector<W::Read>& before,unsigned oldCompletions){
  if(io.completions!=oldCompletions){assert(io.completions==oldCompletions+before.size() && io.pending.empty());
   for(const auto& p:before)for(unsigned at=0;at<p.bytes;at+=4){const unsigned a=p.destination+at;uint32_t word=0;
    for(unsigned j=0;j<4;j++)word|=uint32_t(io.l1[a+j])<<(j*8);bytes.word(bytes.context,worker,a)=word;}}
  uint32_t delta[8];unsigned mask=0;bool changed=false;
  for(unsigned i=0;i<8;i++){const uint32_t now=io.counters[Index(i)];delta[i]=now-published[i];published[i]=now;changed|=delta[i]!=0;}
  for(unsigned i=0;i<10;i++){if(commands[i]!=io.command[i])mask|=1U<<i;commands[i]=io.command[i];}
  const bool next=!io.pending.empty();if(changed || mask || next!=pending)bytes.events(bytes.context,worker,delta,commands,mask,next!=pending,next);pending=next;
 }
};
Q38NativeBfp4PeerBridge::Q38NativeBfp4PeerBridge(const uint32_t job[16],unsigned worker,const Q38NativePeerTestBytes& bytes):impl(new Impl(job,worker,bytes)){}
Q38NativeBfp4PeerBridge::~Q38NativeBfp4PeerBridge(){delete impl;}
bool Q38NativeBfp4PeerBridge::Handles(uint32_t a)const{
 return a==0xffe40000 || a==0xffe000fc || (a>=0xffef0000 && a<0xffef0000+224*4)
  || (a>=0xffbd8800 && a<0xffbd8840) || (a>=0xffbd8a00 && a<0xffbd8a40)
  || (a>=0xffb20000 && a<0xffb22000) || impl->io.Header(a) || impl->io.InputScratch(a);
}
uint32_t Q38NativeBfp4PeerBridge::Read(uint32_t a){
 auto& x=*impl;assert(!(a&3) && Handles(a));if(a==0xffe000fc)return x.ack;
 if(a>=0xffef0000 && a<0xffef0000+224*4){assert(x.Loaded() && !x.native.Pending() && !x.native.mathPending);return x.native.config[(a-0xffef0000)/4];}
 if((a>=0xffbd8800 && a<0xffbd8840) || (a>=0xffbd8a00 && a<0xffbd8a40)){
  assert(x.native.Clean() && x.Loaded());const unsigned row=(a-0xffbd8000)/64,col=(a%64)/4;
  assert(x.native.valid[row] && (row==32 || row==40));x.native.outputReads++;return NativeBfpMachine::Bits(x.native.destination[row][col]);}
 const auto before=x.io.pending;const unsigned completed=x.io.completions;const uint32_t value=x.io.Read(a);x.Publish(before,completed);return value;
}
void Q38NativeBfp4PeerBridge::Write(uint32_t a,uint32_t value){
 auto& x=*impl;assert(!(a&3) && Handles(a));
 if(a==0xffe40000){assert(x.Loaded());if(value==0xa2101816){assert(!x.ack && !x.awaiting);x.awaiting=true;}
  else if(value==0x4500017e){assert(x.awaiting && !x.ack);x.native.Drain();x.ack=1;x.awaiting=false;}else x.native.Op(value);return;}
 if(a==0xffe000fc){assert(!value && x.Loaded());x.ack=0;return;}
 if(a>=0xffef0000 && a<0xffef0000+224*4){assert(x.Loaded());x.native.Config((a-0xffef0000)/4,value);return;}
 const auto before=x.io.pending;const unsigned completed=x.io.completions;x.io.Write(a,value);
 if(a<0x180000)x.bytes.word(x.bytes.context,x.worker,a)=value;x.Publish(before,completed);
}
unsigned Q38NativeBfp4PeerBridge::MatrixInstructions()const{return impl->native.matrixOps;}
bool Q38NativeBfp4PeerBridge::Clean()const{return impl->Loaded() && impl->native.Clean() && !impl->awaiting;}
uint32_t Q38NativeBfp4PeerBridge::CounterDelta(unsigned i)const{assert(i<8 && Clean());return impl->published[i]-impl->initial[i];}
uint32_t Q38NativeBfp4PeerBridge::LastCommand(unsigned i)const{assert(i<10 && Clean());return impl->commands[i];}
