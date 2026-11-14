/* SPDX-License-Identifier: MIT */
// Inside the actual experimental parent. Always start with immutable pos0
// templates, transform ON CARD, and stage before the real returning call.
#include "PioneerQ38RedistributedTokenLayout.h"
static __attribute__((noinline,section(".conv.token"))) void TokenRecipes(unsigned from,unsigned to,unsigned first,unsigned count,bool gddr){
 const auto* p=Owner();
 for(unsigned slot=first;slot<first+count;slot++){
  Transfer(from+(slot-first)*5120,5120,false);
  if(!M::Recipe(reinterpret_cast<uint32_t*>(R::Scratch),p->state[Layer],slot,p->state[Position],p->state[Token]))scalar_fault_park();
  if(gddr)Transfer(to+(slot-first)*5120,5120,true);
  else for(unsigned i=0;i<1280;i++)Words(to+(slot-first)*5120)[i]=Words(R::Scratch)[i];
 }
}
static __attribute__((noinline,section(".conv.token"))) void Metadata(unsigned layer,unsigned package){
 const auto* p=Owner();const auto* o=p->outer;const unsigned position=p->state[Position];
 if(!L::Dense(layer)){
  TokenRecipes(package+L::P::RecipesOffset,o[57],0,8,true);
  unsigned worker=0,slot=0;
  for(unsigned at=0;at<L::P::ConvBytes;at+=16384){
   unsigned bytes=L::P::ConvBytes-at;if(bytes>16384)bytes=16384;
   Transfer(package+L::P::ConvOffset+at,bytes,false);
   for(unsigned i=0;i<bytes;i+=256){
    if(!M::Conv(reinterpret_cast<uint32_t*>(R::Scratch+i),layer,worker,slot,position))scalar_fault_park();
    if(++slot==6){slot=0;worker++;}
   }
   Transfer(o[58]+at,bytes,true);
  }
  if(worker!=120 || slot)scalar_fault_park();
  namespace RT=PioneerQ38RedistributedTokenLayout;
  uint32_t candidate[RT::HeaderWords];bool routed=false,hasCandidate=false;
  Transfer(package+RT::Offset,RT::HeaderWords*4,false);
  for(unsigned i=0;i<RT::HeaderWords;i++){
   candidate[i]=Words(R::Scratch)[i];hasCandidate|=candidate[i]!=0;
  }
  if(hasCandidate){
   if(candidate[0]!=RT::Magic || candidate[1]!=RT::Version || candidate[2]!=RT::TableWords
    || candidate[3]!=layer || candidate[4]!=0 || candidate[5]!=o[7] || candidate[6]!=0
    || candidate[7]!=M::Phase(0,layer)+4 || candidate[8]!=layer+1 || candidate[9]!=96
    || candidate[10]!=RT::Workers || candidate[11]!=48)scalar_fault_park();
   for(unsigned i=12;i<16;i++)if(candidate[i])scalar_fault_park();
   for(unsigned i=0;i<8;i++)if(candidate[16+i]!=p->boot[8+i])scalar_fault_park();
   for(unsigned i=24;i<RT::HeaderWords;i++)if(candidate[i])scalar_fault_park();
   routed=true;
  }
  const unsigned rows=routed?RT::Workers:RT::LogicalTasks;
  for(unsigned at=0,first=0;first<rows;at+=12288,first+=16){
   unsigned count=rows-first;if(count>16)count=16;
   const unsigned bytes=count*RT::RecordWords*4;
   Transfer(routed?package+RT::Offset+RT::HeaderWords*4+at:package+L::P::GdnOffset+at,bytes,false);
   if(routed){
    for(unsigned i=0;i<count;i++){
     auto* record=reinterpret_cast<uint32_t*>(R::Scratch+i*RT::RecordWords);
     bool empty=true;for(unsigned w=0;w<RT::RecordWords;w++)empty&=record[w]==0;
     if(!empty && !M::GdnRedistributed(record,layer,first+i+1,position))scalar_fault_park();
    }
   }else for(unsigned i=0;i<count;i++)
    if(!M::Gdn(reinterpret_cast<uint32_t*>(R::Scratch+i*RT::RecordWords),layer,first+i,position))scalar_fault_park();
   Transfer(o[59]+at,bytes,true);
  }
  uint32_t* control=reinterpret_cast<uint32_t*>(R::Scratch);
  for(unsigned i=0;i<RT::ControlWords;i++)control[i]=0;
  control[0]=RT::ControlMagic;control[1]=1;control[2]=routed?1:0;
  control[3]=o[3];control[4]=o[4];control[5]=o[7];control[6]=o[6]+5;control[7]=o[48];
  for(unsigned i=0;i<8;i++)control[8+i]=o[32+i];
  Transfer(o[59]+RT::TableWords*4,RT::ControlBytes,true);
 }else{
  TokenRecipes(package+L::DenseRecipes,D::Recipes,0,4,false);
  TokenRecipes(package+L::DenseFfn,o[68],4,3,true);
  for(unsigned at=0;at<96*256;at+=16384){
   unsigned bytes=96*256-at;if(bytes>16384)bytes=16384;
   Transfer(package+L::DenseFrames+at,bytes,false);
   for(unsigned i=0;i<bytes;i+=256)if(!M::Dense(reinterpret_cast<uint32_t*>(R::Scratch+i),layer,(at+i)>>8,position))scalar_fault_park();
   for(unsigned i=0;i<bytes/4;i++)Words(D::Frames+at)[i]=Words(R::Scratch)[i];
  }
 }
 Scope();
}
