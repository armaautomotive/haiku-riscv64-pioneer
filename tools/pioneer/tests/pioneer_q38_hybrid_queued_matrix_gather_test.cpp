/* SPDX-License-Identifier: MIT */
// Actual ROOT BF matrix gather against an actual QGW7 projection worker.
// The NoC endpoint is synthetic; no physical/model/throughput claim.
#define PIONEER_Q38_HYBRID_MATRIX_NOC_MACHINE_ONLY
#include "pioneer_q38_hybrid_matrix_noc_test.cpp"
#include "../PioneerQ38Bf16RootPairs.h"

struct MatrixRoot:PacketRoot {
 unsigned queuedStart=0,queuedEnd=0,queuedReads=0;bool badPeerCounter=false;
 MatrixRoot(const char* root,TransitionImage& worker,unsigned start,unsigned end)
  :PacketRoot(root,worker),queuedStart(start),queuedEnd(end){}
 unsigned Load(unsigned a)override{
  if(a==0xffb20064U)return 0x10101010U;
  if(a==0xffb20068U)return 0;
  return PacketRoot::Load(a);
 }
 unsigned PeerRead(unsigned a)override{
  if(!peer.denseMode && (Range(a,0x83f00,256) || Range(a,0x84000,1024)))return peer.Get(a);
  return PacketRoot::PeerRead(a);
 }
 void Submit()override{
  if(pc>=queuedStart && pc<queuedEnd){
   const unsigned local=command[3],remote=command[0];bool found=false;
   assert(!command[1] && command[2]==peer.boot[4] && !command[4] && command[5]==129
    && !command[6] && !command[7] && command[8]==4 && !command[9]);
   const unsigned indices[]={2,3,4,5,1,8,4,10};
   for(unsigned i=0;i<8;i++)if(remote==0xffb20000U+CounterOffset(i)
    && local==DR::Scratch+i*32+(CounterOffset(i)&31)){
    memory[local]=peer.transfer.counters[indices[i]];
    if(badPeerCounter && i==0)memory[local]^=1;found=true;break;
   }
   assert(found);queuedReads++;packets++;reads++;
   counters[0]++;counters[1]++;counters[2]++;counters[3]++;counters[6]++;return;
  }
  PacketRoot::Submit();
 }
 void Configure(const unsigned* before){
  using BFR=PioneerQ38Bf16RootPairs;
  memory[Frame+6*4]=2025;memory[Frame+9*4]=129;memory[Frame+11*4]=5120;
  memory[Frame+14*4]=17408;memory[Frame+15*4]=12;memory[Frame+40*4]=0;
  memory[Frame+42*4]=0;memory[Frame+43*4]=4;
  for(unsigned i=0;i<8;i++)memory[BFR::Parts+Ordinal*32+i*4]=memory[BFR::Shadow+Ordinal*32+i*4]=
   i==0?0x04000000U:i==2?4U:i==4?786U:0U;
  memory[BFR::Rows+Ordinal*4]=memory[BFR::RowShadow+Ordinal*4]=0;
  for(unsigned i=0;i<7;i++)memory[0x16c400+(14+i)*4]=memory[0x16da00+i*4]=0x12345000+i*64;
  for(unsigned i=0;i<8;i++)memory[0x122000+Ordinal*32+i*4]=before[i];
  memory[0x123000+Ordinal*4]=1;
 }
 bool Gather(unsigned address){
  std::fill(std::begin(regs),std::end(regs),0);regs[1]=0x60004;regs[2]=0x70000;regs[27]=Private;
  regs[10]=Frame;regs[11]=Tracked;regs[12]=Ordinal;pc=address;steps=0;
  while(pc!=0x60004 && pc!=0x60000){Step();assert(regs[2]>=0x6e000 && regs[2]<=0x70000);}
  return pc==0x60004;
 }
};

int main(int argc,char** argv){
 assert(argc==6);unsigned oldGather=0,newGather=0,queue=0,queueSize=0;
 std::ifstream entries(argv[5]);entries>>std::hex>>oldGather>>newGather>>queue>>queueSize;assert(entries.good());
 std::array<unsigned,64> expected={};unsigned baselinePackets=0;
 for(unsigned trial=0;trial<3;trial++){
  auto worker=std::make_unique<TransitionImage>(argv[3],argv[4]);worker->Boot();
  const unsigned indices[]={2,3,4,5,1,8,4,10};unsigned before[8];
  for(unsigned i=0;i<8;i++)before[i]=worker->transfer.counters[indices[i]];
  worker->Stage(0,1,4);assert(worker->FinishCombined(1));
  const bool queued=trial!=0;auto root=std::make_unique<MatrixRoot>(queued?argv[2]:argv[1],*worker,queued?queue:0,queued?queue+queueSize:0);
  root->Configure(before);root->badPeerCounter=trial==2;
  const bool okay=root->Gather(queued?newGather:oldGather);
  if(trial==2){
   assert(!okay && root->queuedReads==8);
   for(unsigned i=0;i<64;i++)assert(!root->memory.count(0x80000+i*4));
   for(unsigned i=0;i<8;i++)assert(root->memory.at(0x122000+MatrixRoot::Ordinal*32+i*4)==before[i]);
   continue;
  }
  assert(okay && root->queuedReads==(queued?8U:0U));
  const auto* golden=PioneerNativeBfp4BandGolden(17408);assert(golden);
  for(unsigned i=0;i<64;i++){
   const unsigned value=root->memory.at(0x80000+i*4);assert(value==worker->Get(W::Output+i*4) && value==golden[i%32]);
   if(!queued)expected[i]=value;else assert(expected[i]==value);
  }
  if(!queued)baselinePackets=root->packets;else assert(root->packets==baselinePackets);
 }
 puts("PASS actual QGW7 BFP4 worker -> old and queued ROOT BfGatherMatrix:64 exact synthetic outputs,8 queued reads and bad peer counter refused before output/rebase; synthetic fabric, no device or TPS");
}
