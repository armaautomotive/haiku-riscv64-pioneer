/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TWIN_Q8_L1_LEASE_H
#define PIONEER_Q38_TWIN_Q8_L1_LEASE_H
#if defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_FULL_OFFLOAD_NATIVE) || defined(PIONEER_ATTENTION_FREESTANDING)
#error Twin Q8 L1 lease is private diagnostic ownership, not Work or full offload
#endif

#include "../../../../../tools/pioneer/PioneerQ38TwinQ8ProjectionSequence.h"

// The same peer journal as QTP3, plus only the three touched ROOT ranges.
// IO supplies real selected-mapping/exclusive-owner and reset-state
// checks. Nothing in this class grants a mapping, ROM identity, reset,
// GDDR readiness, DMA or PCI ownership. Keep the journal alive on failure.
namespace PioneerQ38TwinQ8L1Lease {
namespace S=PioneerQ38TwinQ8ProjectionSequence;
using Tile=S::Tile;
enum : unsigned {
 Entry=0x6000,RootInput=0xa0000,RootScale=0xab000,
 RootSpans=3,WorkerSpans=8,RootWords=S::K/2+S::Blocks+64,WorkerWords=110601,
 SavedWords=RootWords+WorkerWords
};
struct Span {unsigned address, words;};
static constexpr Span SpanAt(Tile tile,unsigned i)
{
 constexpr Span root[]={{RootInput,S::K/2},{RootScale,S::Blocks},{S::Mail,64}};
 constexpr Span worker[]={{Entry,1},{0x20000,3072},{0x30000,16384},{0x40000,4096},
  {0x60000,16384},{0x70000,61440},{0xb8000,9216},{0x17ffc0,8}};
 return tile==Tile::Root?(i<RootSpans?root[i]:Span{}):(i<WorkerSpans?worker[i]:Span{});
}
static constexpr unsigned SpanCount(Tile tile){return tile==Tile::Root?RootSpans:WorkerSpans;}
static constexpr bool In(unsigned at,Span span)
{
 return !(at&3) && span.words && at>=span.address && at-span.address<span.words*4;
}
static constexpr bool Owns(Tile tile,unsigned at)
{
 if(tile!=Tile::Root && tile!=Tile::Worker)return false;
 for(unsigned i=0;i<SpanCount(tile);i++)if(In(at,SpanAt(tile,i)))return true;
 return false;
}
static constexpr bool RuntimeRead(Tile tile,unsigned at)
{
 if(tile==Tile::Root)return Owns(tile,at);
 return In(at,{S::Boot,64}) || In(at,{S::Job,64}) || at==S::Bell || In(at,{S::Status,16})
  || In(at,{S::Proof,64}) || In(at,{S::Output,S::Rows});
}
static constexpr bool RuntimeWrite(Tile tile,unsigned at)
{
 if(tile==Tile::Root)return Owns(tile,at);
 return In(at,{S::Job,64}) || at==S::Bell; // no live boot, code, status, proof or output write
}
static constexpr bool CounterRead(Tile tile,unsigned at)
{
 if(tile==Tile::Root){for(unsigned i=0;i<4;i++)if(at==S::Slave(i))return true;}
 else for(unsigned i=0;i<8;i++)if(at==S::Counter(i))return true;
 return false;
}
static_assert(RootWords==2784 && SavedWords==113385,"exact heap journal for two participants");
static_assert(Owns(Tile::Worker,S::Status) && Owns(Tile::Worker,S::Proof+252)
 && Owns(Tile::Worker,S::Output+(S::Rows-1)*4) && !Owns(Tile::Root,S::Mail+256),"private footprint");

// Existing QGW3 ROM section capacities. ROM adoption writes just the
// authenticated section bytes, preserving unused code padding until STOP.
struct CodeSpan {unsigned address, bytesMax;};
static constexpr CodeSpan CodeAt(unsigned index)
{
 constexpr CodeSpan code[]={{0x20000,0x3000},{0x30000,0x10000},{0x40000,0x4000},
  {0x60000,0x6000},{0x68000,0x6000},{0x70000,0x700},{0x70b00,0x1500},
  {0x72000,0x2000},{0xb8000,0x9000},{Entry,4}};
 return index<10?code[index]:CodeSpan{};
}

enum class Phase { New, Capture, Held, Installing, Ready, Released, Restoring, Closed, Retained };
// Record data only; outer owner interprets requests and handles quarantine.
struct Receipt {
 unsigned op=0,ordinal=0,address=0,seen=0,expected=0;
 Tile tile=Tile::Root;
};
class Lease {
 uint32_t* words;
 unsigned count;
 Phase phase=Phase::New;
 unsigned attempted=0;
 Receipt detail={};

 bool Fail(){phase=Phase::Retained;return false;}
 // All arbitrary originals are checked a second time before installation.
 // Never read again, restore or unmap after the first failed observation.
 template<class IO> bool Journal(IO& io,unsigned op){
  unsigned at=0;detail.op=op;
  for(unsigned iTile=0;iTile<2;iTile++)for(unsigned s=0;s<SpanCount(iTile?Tile::Worker:Tile::Root);s++){
   const Tile tile=iTile?Tile::Worker:Tile::Root;
   const auto span=SpanAt(tile,s);detail.tile=tile;detail.address=span.address;detail.ordinal=at;
   if(!io.Held(tile))return Fail();
   for(unsigned i=0;i<span.words;i++,at++){
    const unsigned address=span.address+i*4;
    detail.address=address;detail.ordinal=at;
    if(op==3){if(!io.WriteHeld(tile,address,words[at]))return Fail();}
    else {
     uint32_t v=0;
     if(!io.ReadHeld(tile,address,v))return Fail();
     if(op==1)words[at]=v;
     else if(words[at]!=v){detail.seen=v;detail.expected=words[at];return Fail();}
    }
   }
   if(!io.Held(tile))return Fail();
  }
  return at==count || Fail();
 }
 template<class IO> bool InstallWord(IO& io,unsigned address,uint32_t value){
  detail.address=address;detail.expected=value;
  if(!io.WriteHeld(Tile::Worker,address,value) || !io.ReadHeld(Tile::Worker,address,detail.seen)
   || detail.seen!=value)return Fail();
  return true;
 }
public:
 Lease(uint32_t* buffer,unsigned size):words(buffer),count(size){}
 Lease(const Lease&)=delete;Lease& operator=(const Lease&)=delete;
 Phase State()const{return phase;}
 const Receipt& Detail()const{return detail;}
 void Retain(){phase=Phase::Retained;} // memory only
 template<class IO> bool Capture(IO& io){
  if(phase!=Phase::New || !words || count!=SavedWords || (reinterpret_cast<uintptr_t>(words)&3))return false;
  phase=Phase::Capture;
  if(!Journal(io,1) || !Journal(io,2))return false;
  phase=Phase::Held;return true;
 }
 // ROM contains ten independently authenticated exact sections, not an
 // old diagnostic shader or arbitrary client data. Section(index,words,n)
 // returns the immutable packed arm chosen by the native admission layer.
 template<class IO,class ROM> bool Install(IO& io,const ROM& rom,S::Arm arm,const S::Binding& binding){
  if(phase!=Phase::Held || !S::Shape(arm,binding))return false;
  const uint32_t* code[10]={};unsigned lengths[10]={};
  for(unsigned i=0;i<10;i++){
   const auto span=CodeAt(i);
   if(!rom.Section(arm,i,code[i],lengths[i]) || !code[i] || (reinterpret_cast<uintptr_t>(code[i])&3)
    || !lengths[i] || lengths[i]>span.bytesMax/4 || (i==9 && lengths[i]!=1)
    || !Owns(Tile::Worker,span.address) || !Owns(Tile::Worker,span.address+(lengths[i]-1)*4))return false;
  }
  phase=Phase::Installing;detail={};detail.op=5;detail.tile=Tile::Worker;
  if(!io.Held(Tile::Root) || !io.Held(Tile::Worker))return Fail();
  for(unsigned i=0;i<9;i++){
   const auto span=CodeAt(i);
   for(unsigned w=0;w<lengths[i];w++)if(!InstallWord(io,span.address+w*4,code[i][w]))return false;
   if(!io.Held(Tile::Worker))return Fail();
  }
  for(unsigned i=0;i<64;i++)
   if(!InstallWord(io,S::Boot+i*4,binding.boot[i]) || !InstallWord(io,S::Job+i*4,0))return false;
  if(!InstallWord(io,S::Bell,0))return false;
  for(unsigned i=0;i<16;i++)if(!InstallWord(io,S::Status+i*4,0))return false;
  for(unsigned i=0;i<64;i++)if(!InstallWord(io,S::Proof+i*4,0))return false;
  for(unsigned i=0;i<S::Rows;i++)if(!InstallWord(io,S::Output+i*4,0x7fc00000))return false;
  if(!io.Held(Tile::Worker) || !InstallWord(io,Entry,code[9][0]) || !io.Held(Tile::Worker))return Fail();
  phase=Phase::Ready;return true;
 }
 bool NoteRelease(){
  if(phase!=Phase::Ready || attempted)return false;
  attempted=1;phase=Phase::Released;return true; // BEFORE actual release
 }
 // The outer owner must independently admit STOP + reset-held, then pass
 // that same owner via ClosedWorkerHeld. No magic number or zeroed status
 // by itself authorizes restoring an image that may still be executing.
 template<class IO> bool Restore(IO& io){
  if(phase!=Phase::Released)return false;
  if(!io.ClosedWorkerHeld())return Fail();
  phase=Phase::Restoring;
  if(!Journal(io,3) || !Journal(io,4))return false;
  phase=Phase::Closed;return true;
 }
};
}
#endif
