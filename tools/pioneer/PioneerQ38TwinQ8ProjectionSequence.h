/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TWIN_Q8_PROJECTION_SEQUENCE_H
#define PIONEER_Q38_TWIN_Q8_PROJECTION_SEQUENCE_H
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) \
 || defined(PIONEER_ATTENTION_FREESTANDING)
#error Twin Q8 projection sequence is a separate diagnostic only
#endif
#include <stdint.h>
#include "PioneerQ38TwinNativeQ8Wire.h"

// Single small A/B projection. No device, MMIO mapping, arbitrary job,
// software fallback, reset, or release capability. The future adapter must
// own a saved ROOT+worker lease, exact linked arm, verified fixture upload,
// and clean initial worker boot. The ROOT is held, with no competing traffic.
namespace PioneerQ38TwinQ8ProjectionSequence {
namespace W=PioneerQ38TwinNativeQ8Wire;
enum class Arm : unsigned { Raw=W::Raw, Native=W::Native };
enum class Tile { Root, Worker };
enum class Stage : unsigned { Empty, Admission, Baseline, MailboxClear, Input,
 MatrixStage, MatrixBell, MatrixWait, MatrixSeal, PushReceipt, MatrixProof,
 Output, StopStage, StopBell, StopWait, StopSeal, Close, Complete };
enum class Error : unsigned { None, State, Binding, Read, Write, DeviceFault,
 Word, Counter, PollLimit, Clock, CleanClose };
enum : unsigned {
 Worker=69, Peer=(7|(11<<6)), Root=129, Groups=3, Rows=48, K=5120, Blocks=160,
 WeightBytes=Groups*Blocks*W::Tile, WeightPages=(WeightBytes+4095)/4096,
 Route=786, Magic=0x33574751, ReceiptMagic=0x33504d51, MatrixMagic=0x31483851,
 Boot=0x75000, Job=0x75100, Bell=0x75200, Status=0x75240, Proof=0x83f00, Output=0x84000,
 Mail=0x118000+(Worker-1)*256, ReadCount=122, ReadFlits=4490, Panels=Groups*K/16
};
static_assert(WeightBytes==276480 && WeightPages==68 && ReadFlits==(WeightBytes+K*2+Blocks*4)/64,
 "exact paired shard and shared activation traffic");

struct Binding {
 uint32_t boot[64]={}, address=0;
};
struct Result {
 Stage stage=Stage::Empty;Error error=Error::None;
 uint32_t at=0,seen=0,expected=0,polls=0,unsafe=0,stopped=0,closed=0;
 uint64_t matrixMicros=0;
 uint32_t output[Rows]={};
};
static inline bool Valid(Arm arm){return arm==Arm::Raw || arm==Arm::Native;}
static inline bool Shape(Arm arm,const Binding& a)
{
 const auto* b=a.boot;
 if(!Valid(arm) || a.address<0x04000000 || a.address>0xc4000000U-WeightPages*4096
  || (a.address&4095) || b[0]!=Magic || b[1]!=1 || b[2]!=64 || b[3]!=Worker || b[4]!=Peer
  || !b[5] || b[5]!=b[16] || b[6]!=8192 || b[7]!=64)return false;
 for(unsigned i=8;i<16;i+=2)if(!(b[i]|b[i+1]))return false;
 for(unsigned i=32;i<40;i++)if(b[i])return false;
 constexpr unsigned ext[]={0x334f4751,1,K,248320,12,1,0x31415051,68,256,
  ReceiptMagic,Root,0x118000,256,0x83c00,64};
 for(unsigned i=0;i<15;i++)if(b[40+i]!=ext[i])return false;
 if(b[55]!=(arm==Arm::Native?unsigned(W::BootTag):0U))return false;
 for(unsigned i=56;i<64;i++)if(b[i])return false;
 return true; // DATA shape, not evidence that code or storage is bound
}
static inline void JobWords(Arm arm,const Binding& a,bool stop,uint32_t (&job)[64])
{
 for(unsigned i=0;i<64;i++)job[i]=0;
 const unsigned head[]={Magic,stop?2U:1U,stop?2U:1U,Worker,Peer,0,0,stop?9999U:2025U};
 for(unsigned i=0;i<8;i++){job[i]=head[i];job[8+i]=a.boot[8+i];}
 if(!stop){
  const unsigned capsule[]={MatrixMagic,8,W::Tile,0,0,K,a.address,Groups,Route,1,unsigned(arm)};
  for(unsigned i=0;i<11;i++)job[16+i]=capsule[i];
 }
}
static inline unsigned Counter(unsigned i){
 constexpr unsigned v[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
 return v[i];
}
static inline unsigned Slave(unsigned i){
 constexpr unsigned v[]={0x2f0,0x2e0,0x2e8,0x2c4};
 return v[i];
}
static inline unsigned ReadDelta(unsigned i){return i==1?ReadFlits:i==0 || i==2 || i==3 || i==6?ReadCount:0U;}
static inline unsigned WriteDelta(unsigned i){return i==5?5U:i==2 || i==4 || i==6 || i==7?2U:0U;}

// Adapter methods: Begin(arm,binding) proves exact image/fixture/owner and
// starts no work; Read/Write address are L1 only; Count accepts only named NoC
// offsets. PrepareInput writes/verifies just the fixed activation pattern.
// FinishStopped proves the exact twice-observed stopped PC and restores the
// already-held lease; it MUST retain on uncertainty. Retain is memory-only.
// Poll is a bounded delay/yield; Now is a monotonic host clock (microseconds).
template<class IO> bool Run(IO& io,Arm arm,unsigned variant,const Binding& bind,Result& r)
{
 if(r.stage!=Stage::Empty)return false;
 r={};
 r.stage=Stage::Admission;
 auto fail=[&](Error why,unsigned at=0,unsigned seen=0,unsigned expected=0){
  r.error=why;r.at=at;r.seen=seen;r.expected=expected;
  r.unsafe=1;io.Retain();return false;
 };
 if(variant>1 || !Shape(arm,bind)){r.error=Error::Binding;return false;}
 if(!io.Begin(arm,bind))return fail(Error::Binding);
 auto read=[&](Tile t,unsigned at,uint32_t& v){
  return io.Read(t,at,v) || fail(Error::Read,at);
 };
 auto eq=[&](Tile t,unsigned at,unsigned wanted){
  uint32_t v=0;return read(t,at,v) && (v==wanted || fail(Error::Word,at,v,wanted));
 };
 auto wr=[&](Tile t,unsigned at,unsigned v){
  return io.Write(t,at,v) || fail(Error::Write,at);
 };
 auto worker=[&](unsigned at,unsigned wanted){return eq(Tile::Worker,at,wanted);};
 auto fault=[&](){
  uint32_t v=0;return read(Tile::Worker,Status+8,v) && (!v || fail(Error::DeviceFault,Status+8,v));
 };
 auto seal=[&](unsigned seq,unsigned op){
  const unsigned prefix[]={seq,seq,0,Magic,op,0,0,0};
  if(!fault() || !worker(Bell,seq))return false;
  for(unsigned i=0;i<16;i++)if(!worker(Status+i*4,i<8?prefix[i]:0U))return false;
  for(unsigned i=0;i<64;i++)if(!worker(Boot+i*4,bind.boot[i]))return false;
  return fault();
 };
 auto count=[&](Tile t,unsigned off,uint32_t& v){
  return io.Count(t,off,v) || fail(Error::Read,off);
 };
 auto verifyCount=[&](Tile t,unsigned off,uint32_t expected){
  uint32_t v=0;return count(t,off,v) && (v==expected || fail(Error::Counter,off,v,expected));
 };
 auto writeJob=[&](const uint32_t (&job)[64]){
  for(unsigned i=0;i<64;i++)if(!wr(Tile::Worker,Job+i*4,job[i]))return false;
  for(unsigned i=0;i<64;i++)if(!worker(Job+i*4,job[i]))return false;
  return true;
 };
 auto wait=[&](unsigned seq,uint64_t since,uint64_t& observed){
  uint64_t prev=since;
  for(unsigned i=0;i<150000;i++){
   const uint64_t now=io.Now();
   if(now<prev)return fail(Error::Clock);
   if(now-since>=UINT64_C(30000000))return fail(Error::PollLimit);
   prev=now;r.polls++;
   uint32_t done=0;
   if(!fault() || !read(Tile::Worker,Status+4,done))return false;
   if(done==seq){
    observed=io.Now();
    return observed>=now || fail(Error::Clock);
   }
   if(done!=seq-1)return fail(Error::State,Status+4,done,seq);
   io.Poll();
  }
  return fail(Error::PollLimit);
 };
 r.stage=Stage::Baseline;
 uint32_t begin[8]={},slave[4]={},job[64]={};
 if(!seal(0,0))return false;
 for(unsigned i=0;i<8;i++)if(!count(Tile::Worker,Counter(i),begin[i]))return false;
 if(begin[2]!=begin[6])return fail(Error::Counter,Counter(6),begin[6],begin[2]);
 for(unsigned i=0;i<4;i++)if(!count(Tile::Root,Slave(i),slave[i]))return false;
 r.stage=Stage::MailboxClear;
 for(unsigned i=0;i<64;i++)if(!wr(Tile::Root,Mail+i*4,0))return false;
 for(unsigned i=0;i<64;i++)if(!eq(Tile::Root,Mail+i*4,0))return false;
 r.stage=Stage::Input;
 if(!io.PrepareInput(variant))return fail(Error::Binding);
 if(!seal(0,0))return false;
 r.stage=Stage::MatrixStage;
 JobWords(arm,bind,false,job);if(!writeJob(job))return false;
 r.stage=Stage::MatrixBell;
 const uint64_t started=io.Now();
 if(!wr(Tile::Worker,Bell,1))return false;
 r.stage=Stage::MatrixWait;
 uint64_t ended=0;
 if(!wait(1,started,ended))return false;
 if(ended<=started)return fail(Error::Clock);
 r.stage=Stage::MatrixSeal;
 if(!seal(1,0))return false;
 for(unsigned i=0;i<8;i++)if(!verifyCount(Tile::Worker,Counter(i),begin[i]+ReadDelta(i)+WriteDelta(i)))return false;
 constexpr unsigned pushSlave[]={2,5,2,2};
 for(unsigned i=0;i<4;i++)if(!verifyCount(Tile::Root,Slave(i),slave[i]+pushSlave[i]))return false;
 r.stage=Stage::PushReceipt;
 const unsigned header[]={ReceiptMagic,64,Worker,1,0,0,Peer,Root};
 for(unsigned i=0;i<8;i++)if(!eq(Tile::Root,Mail+i*4,header[i]))return false;
 for(unsigned i=0;i<32;i++)if(!eq(Tile::Root,Mail+(8+i)*4,job[i]))return false;
 for(unsigned i=0;i<16;i++)if(!eq(Tile::Root,Mail+(40+i)*4,bind.boot[16+i]))return false;
 for(unsigned i=0;i<8;i++)if(!eq(Tile::Root,Mail+(56+i)*4,begin[i]+ReadDelta(i)))return false;
 r.stage=Stage::MatrixProof;
 const unsigned finalCommand[]={bind.address+WeightBytes-W::Tile,0,Route,0x90000,0,Peer,0,0,W::Tile,0};
 for(unsigned i=0;i<8;i++)
  if(!worker(Proof+i*4,begin[i]) || !worker(Proof+(8+i)*4,begin[i]+ReadDelta(i)))return false;
 const unsigned fields[]={Panels,1,Worker,0x32504651};
 for(unsigned i=0;i<4;i++)if(!worker(Proof+(16+i)*4,fields[i]))return false;
 for(unsigned i=0;i<10;i++)if(!worker(Proof+(20+i)*4,finalCommand[i]))return false;
 if(!worker(Proof+120,bind.boot[16]) || !worker(Proof+124,Peer))return false;
 for(unsigned i=0;i<16;i++)if(!worker(Proof+(32+i)*4,bind.boot[16+i]))return false;
 if(!worker(Proof+W::ProofIndex*4,W::ProofWord(unsigned(arm))))return false;
 for(unsigned i=W::ProofIndex+1;i<64;i++)if(!worker(Proof+i*4,0))return false;
 r.stage=Stage::Output;
 uint32_t collected[Rows]={};
 for(unsigned i=0;i<Rows;i++){
  const unsigned at=Output+i*4;
  if(!read(Tile::Worker,at,collected[i]))return false;
  if((collected[i]&0x7f800000)==0x7f800000)return fail(Error::Word,at,collected[i]);
 }
 if(!fault())return false;
 r.stage=Stage::StopStage;
 JobWords(arm,bind,true,job);if(!writeJob(job))return false;
 r.stage=Stage::StopBell;
 const uint64_t stopping=io.Now();
 if(stopping<ended)return fail(Error::Clock);
 if(!wr(Tile::Worker,Bell,2))return false;
 r.stage=Stage::StopWait;
 uint64_t stopped=0;
 if(!wait(2,stopping,stopped))return false;
 r.stage=Stage::StopSeal;
 if(!seal(2,2))return false;
 for(unsigned i=0;i<8;i++)if(!verifyCount(Tile::Worker,Counter(i),begin[i]+ReadDelta(i)+WriteDelta(i)))return false;
 for(unsigned i=0;i<4;i++)if(!verifyCount(Tile::Root,Slave(i),slave[i]+pushSlave[i]))return false;
 r.stage=Stage::Close;
 if(!io.FinishStopped())return fail(Error::CleanClose);
 r.stopped=r.closed=1;
 for(unsigned i=0;i<Rows;i++)r.output[i]=collected[i];
 r.matrixMicros=ended-started;r.stage=Stage::Complete;
 return true;
}
}
#endif
