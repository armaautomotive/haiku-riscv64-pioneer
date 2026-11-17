/* SPDX-License-Identifier: MIT */
// Actual linked ROOT instructions and NoC command generation on a synthetic
// fabric. Executes two old one-token controls for every positive pair.
#define PIONEER_Q38_SHARED_ROWS_MACHINE_ONLY
#include "pioneer_qwen38_rows_image_test.cpp"
#include "../PioneerQ38TwoTokenQ4StreamLayout.h"
#include <memory>

namespace L=PioneerQ38TwoTokenQ4StreamLayout;
struct TwoStream:RowsMachine {
 uint32_t command[10]={};
 uint32_t counts[8]={};
 unsigned aUnpacks=0,bUnpacks=0,published=0,fabricFault=0,when=1;
 unsigned input1;
 bool initialFifo=false;
 std::vector<unsigned> addresses;
 explicit TwoStream(const char* image,const StreamMachine& first,const StreamMachine& second,unsigned bank,unsigned secondBase=L::Input1)
  :RowsMachine(image,12,first.inputs,1),input1(secondBase){
  assert(input1==L::Input1 || input1==L::VectorInput1);
  tiles=first.tiles;
  for(unsigned w=0;w<L::Words;w++)memory[L::Descriptor+w*4]=0;
  memory[L::Descriptor]=L::Magic;memory[L::Descriptor+4]=12;memory[L::Descriptor+8]=L::TileBytes;
  memory[L::Descriptor+20]=inputs;memory[L::Descriptor+24]=base;memory[L::Descriptor+28]=bank;
  for(unsigned t=0;t<2;t++){
   const auto& source=t?second:first;
   for(unsigned w=0;w<inputs/2;w++)memory[(t?input1:L::Input0)+w*4]=source.memory.at(L::Input0+w*4);
   for(unsigned b=0;b<tiles.size();b++)memory[L::Scale(t)+b*4]=source.memory.at(L::Scale0+b*4);
  }
  command[0]=base;command[2]=PioneerQ38ProjectionRoute(bank);command[3]=L::Tile;
  command[5]=L::Self;command[8]=L::TileBytes;
  // Both requests and flits wrap. Non-read counters never move.
  for(unsigned i=0;i<8;i++)counts[i]=0xfffffff8+i;
  counts[6]=counts[2];
 }
 uint32_t Load(uint32_t at)override{
  if(at==0xffb20040){
   doorbellRead=true;
   if(pending && ++waits>delay && requests!=faultAt){
    for(unsigned w=0;w<L::TileBytes/4;w++)memory[L::Tile+w*4]=Word(tiles.at(requests-1),w*4);
    pending=false;responses++;
    for(unsigned i=0;i<8;i++)counts[i]+=L::Delta(i);
    if(requests==when){
     if(fabricFault==1)counts[4]++; // unrelated write-counter delta
     if(fabricFault==2)command[5]^=1; // retarget after command release
     if(fabricFault==3)memory[L::Descriptor+28]^=1; // changed admitted identity
    }
   }
   return pending?1:0;
  }
  if(at==0xffb20240)return pending || (initialFifo && !requests)?1:0;
  for(unsigned i=0;i<8;i++)if(at==0xffb20000+L::Counter(i))return counts[i];
  if(Range(at,0xffb20000,40)){
   if(at==0xffb20000)targetRead=true;
   return command[(at-0xffb20000)/4];
  }
  if(Range(at,L::Descriptor,L::Words*4) || Range(at,input1,inputs*2) ||
   Range(at,L::Scale1,unsigned(tiles.size()*4)))return memory.at(at);
  return StreamMachine::Load(at);
 }
 void Store(uint32_t at,uint32_t value)override{
  if(at==0xffb20000){
   assert(drained && !pending && requests<tiles.size() && value==base+requests*L::TileBytes);
   command[0]=target=value;targetRead=false;return;
  }
  if(at==0xffb20040){
   assert(value==1 && drained && !pending && targetRead && requests<tiles.size());
   assert(command[0]==base+requests*L::TileBytes && command[2]==PioneerQ38ProjectionRoute(memory.at(L::Descriptor+28)));
   addresses.push_back(command[0]);requests++;pending=true;doorbellRead=false;waits=0;return;
  }
  if(Range(at,L::Output,0x4000)){
   assert(Range(at,L::Output,128) && matrixInstructions==MatrixPhasesPerPanel()*(inputs/16) && drained
    && requests==tiles.size() && responses==requests && !pending);
   published++;RuntimeMachine::Store(at,value);return;
  }
  StreamMachine::Store(at,value);
 }
 void Instruction(uint32_t op)override{
  StreamMachine::Instruction(op);
  if(op==0x420000c1)aUnpacks++;
  if(op==0x428000c1){
   bUnpacks++;
   for(unsigned row=2;row<8;row++)for(unsigned k=0;k<16;k++)assert(b[row][k]==0);
  }
 }
 bool RunPair(unsigned magic=L::Magic,unsigned fault=L::Fault){
  while(pc!=0x72038 && pc!=0x72058)StepBlock();
  assert(minStack>=0x6e000);
  if(pc==0x72058){assert(l1[Index(0x17ffc8)]==fault);return false;}
  assert(l1[Index(0x17ffc0)]==0x7200c && l1[Index(0x17ffc8)]==magic);
  return l1[Index(0x17ffc4)]==inputs/16;
 }
 void Verify(const StreamMachine& first,const StreamMachine& second)const{
  assert(requests==responses && requests*2==first.requests+second.requests && addresses.size()==tiles.size());
  assert(aUnpacks==inputs/16 && bUnpacks==aUnpacks && matrixInstructions==first.matrixInstructions);
  assert(outputReads==inputs*2 && published==32);
  for(unsigned n=0;n<16;n++){
   assert(buffers[4096+n]==first.buffers[4096+n]);
   assert(buffers[4096+16+n]==second.buffers[4096+n]);
  }
  for(unsigned n=32;n<4096;n++)assert(buffers[4096+n]==0x7fc00000);
 }
};

#ifndef PIONEER_Q38_TWO_STREAM_MODEL_ONLY
int main(int argc,char** argv){
 assert(argc==3);unsigned total=0;
 for(unsigned width:{5120U,6144U,17408U}){
  auto first=std::make_unique<StreamMachine>(argv[1],12,width,1);
  auto second=std::make_unique<StreamMachine>(argv[1],12,width,191);
  second->tiles=first->tiles;
  for(unsigned b=0;b<second->tiles.size();b++){
   second->scales[b]=Bits(b%2?0x1p-32f:.0078125f);
   second->memory[L::Scale0+b*4]=second->scales[b];
  }
  auto pair=std::make_unique<TwoStream>(argv[2],*first,*second,width%7);
  first->delay=3;second->delay=5;pair->delay=11;
  const auto before=pair->memory;
  assert(first->RunStream() && first->CorrectStream() && second->RunStream() && second->CorrectStream());
  assert(pair->RunPair());pair->Verify(*first,*second);
  for(const auto& [address,value]:before)if(TwoStream::Range(address,L::Descriptor,L::Words*4) ||
    TwoStream::Range(address,L::Input0,width*2) || TwoStream::Range(address,L::Input1,width*2) ||
    TwoStream::Range(address,L::Scale0,unsigned(pair->tiles.size()*4)) ||
    TwoStream::Range(address,L::Scale1,unsigned(pair->tiles.size()*4)))assert(pair->memory.at(address)==value);
  printf("PASS full-K Q4_K %u: %u shared reads vs %u separate, 32 exact outputs\n",
   width,pair->requests,first->requests+second->requests);total+=pair->requests;
 }
 auto first=std::make_unique<StreamMachine>(argv[1],12,5120,1);
 auto second=std::make_unique<StreamMachine>(argv[1],12,5120,173);
 unsigned refused=0;
 for(unsigned i=0;i<16;i++){
  auto pair=std::make_unique<TwoStream>(argv[2],*first,*second,3);
  pair->memory[L::Descriptor+i*4]^=i==7?8U:1U;
  assert(!pair->RunPair() && !pair->requests && !pair->published);refused++;
 }
 for(unsigned at:{unsigned(L::Input0),L::Input1+10236,L::Scale0+76,unsigned(L::Scale1)}){
  auto pair=std::make_unique<TwoStream>(argv[2],*first,*second,3);
  pair->memory[at]=at==L::Input0 || at==L::Input1+10236?0:0x7f800000;
  assert(!pair->RunPair() && !pair->requests && !pair->published);refused++;
 }
 for(unsigned count:{1U,10U,20U})for(unsigned fault=0;fault<=3;fault++){
  auto pair=std::make_unique<TwoStream>(argv[2],*first,*second,3);
  if(!fault)pair->faultAt=count;
  else {pair->fabricFault=fault;pair->when=count;}
  assert(!pair->RunPair() && pair->requests==count && !pair->published);
  assert(pair->matrixInstructions==(count-1)*64);refused++;
 }
 for(unsigned variant=0;variant<4;variant++){
  auto pair=std::make_unique<TwoStream>(argv[2],*first,*second,3);
  if(variant==0)pair->initialFifo=true;
  if(variant==1)pair->command[2]^=1;
  if(variant==2)pair->command[8]^=64;
  if(variant==3)pair->memory[L::Descriptor+24]=L::EndAddress-20*L::TileBytes+64;
  assert(!pair->RunPair() && !pair->requests && !pair->published);refused++;
 }
 printf("PASS %u vs %u actual simulated weight requests; %u refused malformed/uncertain cases, no partial output/retry; no hardware timing\n",
  total,total*2,refused);
}
#endif
