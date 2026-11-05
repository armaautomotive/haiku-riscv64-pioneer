/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_PREPARED_EXACT_Q4_IMAGE) || PIONEER_Q38_PREPARED_EXACT_Q4_IMAGE != 1
#error Separate prepared-source complete worker selection required
#endif
#include "PioneerQ38PreparedQ4Layout.h"
#include "PioneerQ38PreparedQ4Mask.h"
#include "PioneerQ38ExactQ4Original.h"
#include "PioneerQ38ExactQ4Affine.h"
#ifdef PIONEER_Q38_Q4_BLOCK_INTEGER_IMAGE
#include "PioneerQ38Q4BlockIntegerImageRole.h"
#include "PioneerQ38Q4BlockIntegerState.h"
#endif
#ifdef PIONEER_Q38_PREPARED_TWO_PRODUCT_IMAGE
#if PIONEER_Q38_PREPARED_TWO_PRODUCT_IMAGE != 1
#error Distinct offboard two-product worker image only
#endif
#include "PioneerQ38ExactQ4TwoProduct.h"
using PreparedQ4Dot=PioneerQ38ExactQ4TwoProductDot;
#ifdef PIONEER_Q38_PREPARED_TWO_PRODUCT_REPLAY_IMAGE
#if PIONEER_Q38_PREPARED_TWO_PRODUCT_REPLAY_IMAGE != 1
#error Distinct offboard replay13 two-product worker only
#endif
#include "PioneerQ38ExactQ4TwoProductReplay.h"
using PreparedQ4Affine=PioneerQ38ExactQ4TwoProductReplay;
#else
using PreparedQ4Affine=PioneerQ38ExactQ4TwoProductAffine;
#endif
#else
#ifdef PIONEER_Q38_PREPARED_TWO_PRODUCT_REPLAY_IMAGE
#error Replay13 worker requires its two-product parent
#endif
using PreparedQ4Dot=PioneerQ38ExactQ4Corrected;
using PreparedQ4Affine=PioneerQ38ExactQ4Affine;
#endif
using PreparedQ4=PioneerQ38PreparedQ4Layout;
#ifdef PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE
#ifdef PIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_IMAGE
#include "PioneerQ38QueuedQ4FixedConfig.h"
#endif
#ifdef PIONEER_Q38_QUEUED_Q4_PAIR_CONFIG_IMAGE
#include "PioneerQ38QueuedQ4PairConfig.h"
#endif
#ifdef PIONEER_Q38_QUEUED_Q4_DELTA_CONFIG_IMAGE
#include "PioneerQ38QueuedQ4DeltaConfig.h"
#endif
#include "PioneerQ38QueuedQ4Block.h"
static_assert(PioneerQ38QueuedQ4Block::EscapeBase-16>=PreparedQ4::DenseMask+512
 && PioneerQ38QueuedQ4Block::EscapeBase-16+PioneerQ38QueuedQ4Block::EscapeBytes<=0xa0000,
 "distinct private escape planes do not overlap masks or original inputs");
#endif
// Source descriptor is at a FIXED root-owned slot,not an arbitrary job
// pointer. A future native ROOT producer must bind its complete lifetime.
static __attribute__((noinline,noclone,section(".text.pioneer_q38_resident_service")))
void PreparedQ4Read(unsigned source,unsigned route,unsigned destination,unsigned bytes,unsigned& reads,unsigned& flits){
 if(!bytes || (bytes&63) || bytes>16384 || (source&63) || (destination&63))block_fault_park();
 ReadCommand(source,route,destination,bytes);FetchBlock(source);reads++;flits+=bytes>>6;
}
static __attribute__((noinline,noclone,section(".text.pioneer_q38_resident_service")))
void PreparedQ4Inputs(unsigned inputs){
 for(unsigned word=0;word<inputs/2;word++){
  const unsigned value=Words(0xa0000)[word];unsigned lo=0,hi=0;
  if(!PioneerQ38ExactQ4Original::Activation(value&65535,lo) || !PioneerQ38ExactQ4Original::Activation(value>>16,hi))block_fault_park();
  Words(PreparedQ4::Input(word))[0]=lo|(hi<<16);
 }
 for(unsigned i=0;i<4;i++){Words(PreparedQ4::InputFirst-16)[i]=0;Words(PreparedQ4::InputSecond-16)[i]=0;}
 // Each B unpack consumes128 BF16 values;only its first16 produce the
 // exported row. Every following value is current-job validated data or
 // explicit zero,including BOTH cache-segment boundaries and job tail.
 for(unsigned i=0;i<56;i++){Words(PreparedQ4::InputFirst+4608)[i]=0;Words(PreparedQ4::InputSecond+inputs*2-4608)[i]=0;}
 Fence();
}
struct PioneerQ38ExactQ4WorkEmitter {
 unsigned metadataRow=128,primary=0x98100,input=PreparedQ4::InputFirst,mask=PreparedQ4::DenseMask;
#ifdef PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE
#ifdef PIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_IMAGE
 PioneerQ38QueuedQ4FixedConfig queue;
#elif defined(PIONEER_Q38_QUEUED_Q4_PAIR_CONFIG_IMAGE)
 PioneerQ38QueuedQ4PairConfig queue;
#elif defined(PIONEER_Q38_QUEUED_Q4_DELTA_CONFIG_IMAGE)
 PioneerQ38QueuedQ4DeltaConfig queue;
#else
 PioneerQ38QueuedQ4Block queue;
#endif
 unsigned queued=0; // fixed RV32 word state,not byte stores
 unsigned escape=PioneerQ38QueuedQ4Block::EscapeBase;
#ifdef PIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_IMAGE
 __attribute__((always_inline)) void BeginQueued(unsigned block,unsigned raw){
  const unsigned lastEscape=PioneerQ38QueuedQ4Block::EscapeBase+15*PioneerQ38QueuedQ4Block::EscapeStride;
  if(queued || !queue.Begin(*this,PreparedQ4::Input(block*128)/16-1,(raw+256)/16-1,
    PreparedQ4::Input(block*128+15*8)/16-1,lastEscape/16-1))block_fault_park();
  queued=true;
 }
#else
 __attribute__((always_inline)) void BeginQueued(){if(queued || !queue.Begin())block_fault_park();queued=true;}
#endif
 __attribute__((always_inline)) bool ReadConfig(unsigned index,unsigned& value){value=Words(0xffef0000+index*4)[0];return true;}
 __attribute__((always_inline)) void EndQueued(){
  if(!queued)block_fault_park();
  ::Drain();queued=false; // real bounded ACK BEFORE any CPU read/reuse
  if(!queue.Verify(*this))block_fault_park();
 }
#endif
 __attribute__((always_inline)) inline void Op(unsigned word){
  if((word&~2U)==0x70130060)word=0x70130000|metadataRow|(word&2);
  else if((word&~2U)==0x70230064)word=0x70230000|(metadataRow+4)|(word&2);
  Emit(word);
 }
 __attribute__((always_inline)) inline void Config(unsigned index,unsigned value){
  if(index==76 && value==0x8fff)value=primary/16-1;
  else if(index==76 && (value==0x900f || value==0x901f))value+=0xc04;
  if(index==124 && value==0x90ff)value=input/16-1;
#ifdef PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE
  if(queued){
   if(index==76 && value==0x901f+0xc04)value=escape/16-1;
   if(!queue.Config(*this,index,value))block_fault_park();
   return;
  }
#endif
  ::Config(index,value);
 }
 void Drain(){
#ifdef PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE
  if(queued){PioneerQ38QueuedQ4Block::Wait(*this);return;}
#endif
  ::Drain();
 }void SealData(){Fence();}
 unsigned Mask(unsigned i){return Words(mask)[i];}
 void Escape(unsigned i,unsigned v){Words(0x9c240)[i]=v;}
 void Seven(unsigned i,unsigned v){Words(0x9c140)[i]=v;}
 unsigned PayloadWord(unsigned at){return Words(PreparedQ4::Payload+at)[0];}
 void MaskStore(unsigned i,unsigned v){Words(PreparedQ4::DenseMask)[i]=v;}
};
static __attribute__((noinline,noclone,section(".text.pioneer_q38_resident_service")))
PreparedQ4::Window PreparedQ4Group(const volatile unsigned* d,unsigned group,unsigned& reads,unsigned& flits){
 PreparedQ4Read(PreparedQ4::IndexSource(d,group),d[7],PreparedQ4::Index,PreparedQ4::IndexReadBytes(d,group),reads,flits);
 PreparedQ4::Window window;
 if(!PreparedQ4::GroupWindow(d,group,Words(PreparedQ4::Index),window))block_fault_park();
 for(unsigned copied=0;copied<window.bytes;){
  const unsigned bytes=window.bytes-copied>16384?16384:window.bytes-copied;
  PreparedQ4Read(d[8]+window.start+copied,d[7],PreparedQ4::Payload+copied,bytes,reads,flits);copied+=bytes;
 }
 return window;
}
__attribute__((noinline,noclone,section(".text.pioneer_q38_resident_service")))
static unsigned PioneerQ38ExactQ4Work(const volatile uint32_t* job){
 const unsigned inputs=job[5],groups=job[7],blocks=inputs>>8,first=job[6],source=job[8];
 if(job[1]!=12 || job[2]!=2304 || job[14]!=1 || !PreparedQ4::RootSlot(job[11]))return 0;
 unsigned reads=0,flits=0;for(unsigned i=0;i<16;i++)Words(PreparedQ4::Summary)[i]=0;Fence();
 PreparedQ4Read(PreparedQ4::RootSlot(job[11]),1|(2<<6),PreparedQ4::Descriptor,64,reads,flits);
 const auto* descriptor=Words(PreparedQ4::Descriptor);if(!PreparedQ4::Shape(descriptor,job))block_fault_park();
 PreparedQ4Inputs(inputs);PioneerQ38ExactQ4WorkEmitter e;
 if(!PreparedQ4Dot::Setup(e))block_fault_park();
 for(unsigned i=0;i<4;i++){Words(0x9c130)[i]=0;Words(0x9c230)[i]=0;}Fence();
 for(unsigned group=0;group<groups;group++){
  e.Drain();const auto window=PreparedQ4Group(descriptor,group,reads,flits);
  e.Config(50,0x100);VectorPanelEmitter init;PioneerQ38VectorDstInit(init,true,true,true,true);
#ifdef PIONEER_Q38_Q4_BLOCK_INTEGER_IMAGE
  PioneerQ38Q4BlockIntegerState integer;VectorPanelEmitter integerEmitter;
  if(!integer.Init(integerEmitter))block_fault_park();
#endif
  uint32_t expected=0;
  for(unsigned block=0;block<blocks;block++){
   const unsigned raw=TwinBatchTilePrefetchStage(12,inputs,groups,group,block,first,source,expected),scale=Words(0x94000)[block];
   const unsigned index=Words(PreparedQ4::Index)[PreparedQ4::IndexWordsBefore(descriptor,group)+block];
   if(!PioneerQ38PreparedQ4Mask::Expand(e,index,window) || !PioneerQ38VectorQ4Metadata::FiniteBlock(Words(raw),2304))block_fault_park();
   e.Drain();e.Config(50,0x100);
#ifdef PIONEER_Q38_Q4_BLOCK_INTEGER_IMAGE
   if(!integer.Begin(integerEmitter,raw))block_fault_park();
#else
   VectorPanelEmitter metadata;
   if(!PioneerQ38VectorQ4Metadata::Block(metadata,raw,scale))block_fault_park();
#endif
   e.Config(1,0x60aa0000);
#ifdef PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE
   // Every correction buffer is immutable until the block's real ACK.
   for(unsigned p=0;p<16;p++){
    const unsigned at=PioneerQ38QueuedQ4Block::EscapeBase+p*PioneerQ38QueuedQ4Block::EscapeStride;
    for(unsigned word=0;word<4;word++)Words(at-16)[word]=0;
    for(unsigned pair=0;pair<8;pair++){
     const unsigned bits=Words(PreparedQ4::DenseMask)[p*8+pair];
     Words(at)[pair*2]=PioneerQ38ExactQ4Corrected::Spread(bits);
     Words(at)[pair*2+1]=PioneerQ38ExactQ4Corrected::Spread(bits>>16);
    }
   }
   Fence();
   // These three fields remain unchanged through all16 panels. Metadata
   // imports occur outside this block and are complete before this point.
   e.Config(65,0x10001);e.Config(73,0xf010f);e.Config(86,0x01000100);
#ifdef PIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_IMAGE
   e.BeginQueued(block,raw);
#else
   e.BeginQueued();
#endif
#endif
   for(unsigned p=0;p<16;p++){
    int sum=0;
    if(group)sum=int(Words(0x95000)[block*16+p]);
    else{
     for(unsigned i=0;i<8;i++){
      const unsigned word=Words(0xa0000)[block*128+p*8+i];
      for(unsigned lane=0;lane<2;lane++){const unsigned v=word>>(lane*16)&65535;sum+=int(v&1023)*(v&0x8000?-1:1);}
     }
     if(groups>1)Words(0x95000)[block*16+p]=unsigned(sum);
    }
    if(sum<-2048 || sum>2048)block_fault_park();
    e.primary=raw+256+p*128;e.input=PreparedQ4::Input(block*128+p*8);e.mask=PreparedQ4::DenseMask+p*32;
    e.Config(124,e.input/16-1);
#ifdef PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE
    PioneerQ38QueuedQ4Block::PanelEmitter<PioneerQ38ExactQ4WorkEmitter> panel(e);
    e.escape=PioneerQ38QueuedQ4Block::EscapeBase+p*PioneerQ38QueuedQ4Block::EscapeStride;
    if(!PioneerQ38QueuedQ4Block::Dot(panel))block_fault_park();
#else
    e.Config(65,0x10001);e.Config(73,0xf010f);e.Config(86,0x01000100);
    PioneerQ38ExactQ4Affine::PanelEmitter<PioneerQ38ExactQ4WorkEmitter> panel(e);
    if(!PreparedQ4Dot::Run(panel))block_fault_park();
#endif
#ifdef PIONEER_Q38_Q4_BLOCK_INTEGER_IMAGE
    if(!integer.Panel(integerEmitter,p,sum))block_fault_park();
#else
    e.metadataRow=128+(p/2)*8;
#ifdef PIONEER_Q38_PREPARED_TWO_PRODUCT_REPLAY_IMAGE
    PreparedQ4Affine::Affine(e,scale,sum,block==0 && p==0);
#else
    PreparedQ4Affine::Affine(e,scale,sum);
#endif
#endif
   }
#ifdef PIONEER_Q38_Q4_BLOCK_INTEGER_IMAGE
   if(!integer.Finish(integerEmitter,scale))block_fault_park();
#endif
#ifdef PIONEER_Q38_QUEUED_Q4_BLOCK_IMAGE
   e.EndQueued();
#endif
  }
  e.Drain();e.Config(3,0);
  for(unsigned n=0;n<16;n++){
   const unsigned value=Words(0xffbd9b00)[n];if((value&0x7f800000)==0x7f800000)block_fault_park();Words(0x84000)[group*16+n]=value;
  }Fence();
 }
 auto* s=Words(PreparedQ4::Summary);s[1]=job[11];s[2]=job[12];s[3]=inputs;s[4]=groups;s[5]=reads;s[6]=flits;Fence();
 s[0]=PreparedQ4::SummaryMagic;Fence();return groups*blocks*16;
}
