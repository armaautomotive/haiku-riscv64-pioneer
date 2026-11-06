/* SPDX-License-Identifier: MIT */
// Actual compiled RV32 ledger instructions. Raw UNIT control inputs only;
// no NoC/worker/native/model execution or completion authority.
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#include <algorithm>
#include <array>
#include <memory>
struct AuditUnitMachine:ScalarMachine {
 std::array<unsigned,4> frame;
 AuditUnitMachine(const char* image,unsigned width,unsigned used,unsigned operation,unsigned index)
  :ScalarMachine(image,4,0),frame{{width,used,operation,index}}{
  for(unsigned i=0;i<4;i++)memory[0x74000+i*4]=frame[i];
 }
 void Check(unsigned begun,unsigned marked,unsigned closed,unsigned refused){
  assert(RunScalar());const unsigned expected[]={begun,marked,closed,refused};
  for(unsigned i=0;i<4;i++){
   assert(memory.at(0x90000+i*4)==expected[i]);
   assert(memory.at(0x74000+i*4)==frame[i]);
  }
  assert(regs[27]==0 && outputWrites==4 && !matrixInstructions && !inputWrites && !configWrites);
  const auto output=outputWrites;for(unsigned i=0;i<32;i++)StepScalar();
  assert(pc==0x64038 && outputWrites==output);
 }
};
int main(int argc,char** argv){
 assert(argc==2);unsigned positives=0,refusals=0,bits=0,stack=0;uint64_t instructions=0;
 const auto trial=[&](unsigned width,unsigned used,unsigned operation,unsigned index,
  unsigned begun,unsigned marked,unsigned closed,unsigned refused){
  auto m=std::make_unique<AuditUnitMachine>(argv[1],width,used,operation,index);
  m->Check(begun,marked,closed,refused);stack=std::max(stack,0x70000-m->minStack);instructions+=m->steps;
 };
 for(unsigned width:{5120U,17408U})for(unsigned used:{1U,19U,20U,21U,31U,32U,33U,63U,64U,65U,67U,68U,69U,95U,96U,97U,118U,119U}){
  trial(width,used,0,0,1,119,1,0);positives++;
  for(unsigned w:{0U,19U,20U,31U,32U,63U,64U,95U,96U,118U}){
   trial(width,used,1,w,1,118,0,0);refusals++;
   trial(width,used,2,w,1,119,0,1);refusals++;
   trial(width,used,3,w,1,w,0,1);refusals++;
  }
  trial(width,used,5,0,1,119,0,1);refusals++;
 }
 for(unsigned bit=0;bit<52*8;bit++){trial(17408,68,4,bit,1,119,0,0);bits++;}
 for(unsigned width:{0U,5119U,5121U,17407U,17409U,UINT32_MAX}){
  trial(width,119,0,0,0,0,0,0);refusals++;
 }
 for(unsigned used:{0U,120U,UINT32_MAX}){trial(5120,used,0,0,0,0,0,0);refusals++;}
 assert(positives==36 && refusals==1125 && bits==416 && stack<=256);
 printf("PASS ACTUAL_RV32_DEFERRED_LEDGER positive%u/refusals%u/singleBitFaults%u/reservedWrittenStack%u/instructions%llu;all119 closure DATA required,both widths,word+partition boundaries,sticky poison;MOCK closure calls,NOT connected worker receipts/ROOT matrix/native/hardware/TPS\n",positives,refusals,bits,stack,(unsigned long long)instructions);
}
