/* SPDX-License-Identifier: MIT */
// Functional queued-control observer ONLY;not a silicon timing model.
#include "pioneer_q38_instruction_prefetch_worker_observer.h"
struct QueuedQ4BlockPeer:InstructionPrefetchPeer {
 using InstructionPrefetchPeer::InstructionPrefetchPeer;
 std::set<unsigned> planes;unsigned planeTile=~0U,queueBlocks=0,queueWaits=0,queueWrites=0;
 bool queued=false;
 static bool Plane(unsigned a){return Range(a,0x9c600,1280);}
 static bool Field(unsigned a){return a==1 || a==50 || a==64 || a==65 || a==72 || a==73 || a==76 || a==84 || a==86 || a==124;}
 void Store(unsigned a,unsigned value)override{
  if(type==12 && exact && Plane(a)){
   assert(!queued && MathIdle() && !(a&3) && maskTile==CurrentTile() && maskDefined.size()==128);
   if(planeTile!=CurrentTile()){planes.clear();planeTile=CurrentTile();}
   memory[a]=value;planes.insert(a);dirty=true;return;
  }
  InstructionPrefetchPeer::Store(a,value);
 }
 unsigned Load(unsigned a)override{
  if(type==12 && exact && Plane(a)){assert(planes.count(a));return memory.at(a);}
  return InstructionPrefetchPeer::Load(a);
 }
 void NativeOperand(const Task& t,unsigned w)override{
  if(type!=12 || w || !t.row){PreparedQ4Peer::NativeOperand(t,w);return;}
  assert(twoProducts && t.row==16 && t.flip && partial[unpack[0]]==1);
  const unsigned panel=(matrixInstructions/2)%16;
  assert(t.address==0x9c610+panel*80 && planeTile==CurrentTile() && planes.size()==320);
  for(unsigned a=t.address-16;a<t.address;a+=4)assert(!memory.at(a));
  // Independent bit-by-bit correction source,not the production Spread.
  for(unsigned i=0;i<256;i++){
   const unsigned bit=(memory.at(L::DenseMask+panel*32+(i/32)*4)>>(i&31))&1;
   assert(((Byte(t.address+i/4)>>((i&3)*2))&3)==bit);
  }
 }
 void Instruction(unsigned op)override{
  const unsigned code=op>>24;
  if(op==0xa2ff9816U || (code>=0xb3 && code<=0xb6)){
   assert(exact && type==12 && !recordRemaining);tick++;
   if(pending){assert(DisjointPrefetchCompute());overlapOps++;}
   if(op==0xa2ff9816U){
    assert(queued);CompleteMath();queueWaits++;return;
   }
   const unsigned index=op&255,mask=(op>>16)&255,byte=(op>>8)&255;
   assert(Field(index) && mask==255 && MathIdle() && !backendWait && thread[0]==0);
   if(!queued){assert(planes.size()==320 && planeTile==CurrentTile());queued=true;queueBlocks++;}
   const unsigned shift=(code-0xb3)*8;
   config[index]=(config[index]&~(255U<<shift))|(byte<<shift);queueWrites++;configWrites++;
   return;
  }
  InstructionPrefetchPeer::Instruction(op);
  if(exact && queued && op==0x4500017e){assert(MathIdle());queued=false;}
 }
 void CheckEnd()override{
  InstructionPrefetchPeer::CheckEnd();assert(!queued);
  if(type==12)assert(queueBlocks==exactJobs*FfnPersistentPeer::groups*Blocks() && queueWaits==queueBlocks*64 && queueWrites);
 }
};
