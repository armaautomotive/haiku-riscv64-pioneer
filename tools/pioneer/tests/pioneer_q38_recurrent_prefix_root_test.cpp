/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RECURRENT_LIFECYCLE_FIXTURE_ONLY
#include "pioneer_q38_mixed_recurrent_block_lifecycle_test.cpp"
#include "../PioneerQ38RecurrentPrefixLayout.h"
namespace XP=PioneerQ38RecurrentPrefixLayout;
namespace XH=PioneerQ38RecurrentHistoryLayout;
struct PrefixSavedState {
 unsigned route,history;std::vector<uint32_t> convolution,state;
 std::array<std::array<unsigned,2>,96> next={};
 explicit PrefixSavedState(const RecurrentLifecycleMachine& m):route(m.outer[8]),history(0),convolution(XH::Bytes/4),state(96*8192){
  assert(XH::Address(m.outer[9],m.outer[3],history));
  for(unsigned i=0;i<convolution.size();i++)convolution[i]=m.Get(route,history+i*4);
  for(unsigned w=0;w<96;w++){
   const auto* d=m.block->Gdn()+w*192+64;next[w]={d[13],d[7]};
   for(unsigned i=0;i<8192;i++)state[w*8192+i]=m.Get(d[13],d[7]+i*4);
  }
 }
 void Verify(const RecurrentLifecycleMachine& m)const{
  for(unsigned i=0;i<convolution.size();i++)assert(m.Get(route,history+i*4)==convolution[i]);
  for(unsigned w=0;w<96;w++)for(unsigned i=0;i<8192;i++)assert(m.Get(next[w][0],next[w][1]+i*4)==state[w*8192+i]);
 }
};
struct AutonomousPrefixMachine:RecurrentLifecycleMachine {
 using Plan=PioneerQ38MixedRecurrentBlockPlan;
 std::array<std::unique_ptr<Plan>,3> prepared;
 std::vector<std::unique_ptr<Plan>> retired;
 std::vector<PrefixSavedState> saved;
 std::array<uint32_t,5120> preceding={};
 unsigned completedBlocks=0,selfOuterWrites=0,selfReleases=0,metadataReadBytes=0,metadataWriteBytes=0,headerReadBytes=0;
 AutonomousPrefixMachine(const char* image,const char* worker):RecurrentLifecycleMachine(image,worker,true,true){
  std::array<uint32_t,1920> sources;
  for(unsigned i=0;i<16;i++)sources[i]=source[i];
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)sources[(w+1)*16+i]=child[w]->source[i];
  prepared[0]=std::move(block);
  const unsigned bank=prepared[0]->Outer()[7],workspace=prepared[0]->Outer()[9],route=prepared[0]->Outer()[8];
  for(unsigned layer=1;layer<3;layer++){
   prepared[layer]=std::make_unique<Plan>();
   assert(prepared[layer]->Build(*canonical,layer,0,0,bank,0,1001+layer*16,layer+1,{1,2,3,4},sources.data(),bank,workspace+RB::FinalOffset));
  }
  // Every primitive learned/current-state byte and immutable metadata package
  // is staged BEFORE the one initial host release. No product is supplied.
  for(unsigned layer=0;layer<3;layer++){
   block=std::move(prepared[layer]);for(unsigned i=0;i<128;i++)outer[i]=block->Outer()[i];
   assert(block->Matches(*canonical));SeedRaw(false,true);
   if(layer)for(unsigned n=0;n<8;n++){
    const auto* r=block->Recipe(n);
    for(unsigned w=0;w<r[7];w++){
     const auto* p=r+80+w*8;unsigned bytes=p[2]*(r[14]/(r[15]==8?32:256))*(r[15]==8?576:2304);
     assert(weights.emplace(Key(p[4],p[0]),Weight{p[4],p[0],bytes,r[15],r[14],r[1032+w]}).second);
    }
   }
   uint32_t package=0;assert(XP::Address(workspace,layer,package));
   for(unsigned i=0;i<128;i++)Put(route,package+i*4,outer[i]);
   for(unsigned n=0;n<8;n++)for(unsigned i=0;i<1280;i++)Put(route,package+XP::RecipesOffset+n*RB::RecipeStride+i*4,block->Recipe(n)[i]);
   for(unsigned i=0;i<RC::ConvWords;i++)Put(route,package+XP::ConvOffset+i*4,block->Conv()[i]);
   for(unsigned i=0;i<RC::GdnWords;i++)Put(route,package+XP::GdnOffset+i*4,block->Gdn()[i]);
   prepared[layer]=std::move(block);
  }
  block=std::move(prepared[0]);for(unsigned i=0;i<128;i++)outer[i]=block->Outer()[i];
  for(unsigned i=40;i<46;i++)memory[RB::Boot+i*4]=boot[i]=XP::BootWord(i);
  assert(XP::BootShape(boot.data()) && !RB::BootShape(boot.data()));
 }
 void Completed(){
  assert(completedBlocks<3 && request==completedBlocks+1 && outer[3]==completedBlocks && !outer[4] && !outer[12] && !outer[13]);
  VerifyBlock();uint32_t history=0;assert(XH::Address(outer[9],outer[3],history));
  const unsigned receipt[]={XH::Marker,history,XH::Bytes,request,outer[3],0,0,1};
  for(unsigned i=0;i<8;i++)assert(memory.at(RB::JointSummary+(56+i)*4)==receipt[i]);
  for(unsigned i=0;i<XH::Bytes/4;i++)assert(Get(outer[8],history+i*4)==Get(outer[8],outer[9]+TT_Q38_CONV_NEXT+i*4));
  for(const auto& previous:saved)previous.Verify(*this);
  saved.emplace_back(*this);
  for(unsigned i=0;i<5120;i++)preceding[i]=Get(outer[8],outer[9]+RB::FinalOffset+i*4);
  completedBlocks++;
  printf("PASS autonomous ROOT completed layer%u/actual preceding hidden/Conv+GDN NEXT retained;NO host metadata or doorbell between layers\n",completedBlocks-1);
 }
 void Store(uint32_t address,uint32_t value)override{
  if(Range(address,RB::Outer,512) && Phase()==16){
   assert(completedBlocks && completedBlocks<=3 && (address-RB::Outer)/4==selfOuterWrites%128);
   unsigned i=(address-RB::Outer)/4,expected=0;
   if(completedBlocks<3)expected=prepared[completedBlocks]->Outer()[i];
   else{
    expected=i==0?unsigned(RB::Magic):i==1?1U:i==2?128U:i==5?129U:i==48?4U:i==49?119U:i==72?unsigned(RB::Stop):0U;
    if(i>=16 && i<32)expected=boot[i];if(i>=32 && i<40)expected=boot[i-24];
   }
   assert(value==expected);memory[address]=value;selfOuterWrites++;return;
  }
  if(address==RB::Doorbell && Phase()==16){
   assert(value==completedBlocks+1 && selfOuterWrites==completedBlocks*128);
   for(unsigned i=0;i<5120;i++)assert(Get(outer[8],outer[9]+RB::FinalOffset+i*4)==preceding[i]);
   memory[address]=value;request=value;selfReleases++;
   if(completedBlocks<3){
    retired.push_back(std::move(block));block=std::move(prepared[completedBlocks]);
    for(unsigned i=0;i<128;i++)outer[i]=block->Outer()[i];
    for(unsigned w=0;w<119;w++){
     assert(!child[w]->active && child[w]->GdnFinished(child[w]->sequence));initial[w]=child[w]->sequence;
     for(unsigned i=0;i<8;i++)peerCounterBaseline[w][i]=child[w]->Count(i);
    }
    // Observer counters ONLY. Never alter a processor PC/register/nonce,
    // Tensix state, metadata packet, current state or completed product.
    matrixStarts=matrixChecks=observed=finalWords=initialNorm=postNorm=activationChecks=gatedChecks=numericCohortChecks=0;
    convStarts=gdnStarts=rootConvPackets=0;cleared.fill(false);verified.fill(false);
   }
   return;
  }
  RecurrentLifecycleMachine::Store(address,value);
  if(address==RB::Status+4 && value && value<=3 && Phase()==14)Completed();
 }
 void Submit()override{
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2],remote=write?command[3]:command[0],bytes=command[8];
  if(route==outer[8]){
   if(Phase()==1){
    uint32_t package=0;assert(XP::Address(outer[9],outer[3],package));
    if(!write && Range(remote,package,512))headerReadBytes+=bytes;
    if(!write && remote>=package+XP::RecipesOffset && uint64_t(remote)+bytes<=uint64_t(package)+XP::Bytes)metadataReadBytes+=bytes;
    if(write && ((remote>=outer[57] && uint64_t(remote)+bytes<=uint64_t(outer[57])+XP::RecipeBytes)
     || (remote>=outer[58] && uint64_t(remote)+bytes<=uint64_t(outer[58])+XP::ConvBytes)
     || (remote>=outer[59] && uint64_t(remote)+bytes<=uint64_t(outer[59])+XP::GdnBytes)))metadataWriteBytes+=bytes;
   }else if(Phase()==16 && !write)headerReadBytes+=bytes;
  }
  RecurrentLifecycleMachine::Submit();
 }
};
#ifndef PIONEER_Q38_RECURRENT_PREFIX_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 auto m=std::make_unique<AutonomousPrefixMachine>(argv[1],argv[2]);assert(m->WaitBlock(0));
 m->memory[RB::Doorbell]=1; // the ONLY host release; every later frame/release is emitted ROOT code
 assert(m->WaitStop());m->VerifyStop();
 assert(m->completedBlocks==3 && m->selfReleases==3 && m->selfOuterWrites==384
  && m->metadataReadBytes==3*(XP::Bytes-512) && m->metadataWriteBytes==m->metadataReadBytes && m->headerReadBytes==5*512);
 for(const auto& previous:m->saved)previous.Verify(*m);
 printf("PASS autonomous prefix ROOT/ONE host start/3actual layers0->1->2/SAME119/ROOT metadata bytes%u+headers%u/ROOT releases%u/earlier Conv+GDN survive/realSTOP;SYNTHETIC raw learned+initial hidden,NOT original/native/physical/current-slot/token/full64/Work/TPS\n",m->metadataReadBytes,m->headerReadBytes,m->selfReleases);
}
#endif
