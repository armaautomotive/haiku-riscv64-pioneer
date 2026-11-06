/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_factored_q4_compare_machine.h"
struct ObservedCompare:FactoredCompareRV32 {
 unsigned writes[2]={},reads[2]={};
 ObservedCompare(const char* path,unsigned width,unsigned bytes):FactoredCompareRV32(path,width,false,bytes){}
 void Store(unsigned a,unsigned v)override{
  if(Range(a,0xffef0000,224*4))writes[native.old?0:1]++;
  FactoredCompareRV32::Store(a,v);
 }
 unsigned Load(unsigned a)override{
  if(Range(a,0xffef0000,224*4))reads[native.old?0:1]++;
  return FactoredCompareRV32::Load(a);
 }
};
int main(int argc,char** argv){
 assert(argc==4);const unsigned bytes=unsigned(std::stoul(argv[3]));assert(bytes && bytes<=16384 && bytes!=11788);
 unsigned words=0;
 for(unsigned width:{5120U,17408U}){
  FactoredCompareData data(width,0);
  auto baseline=std::make_unique<ObservedCompare>(argv[1],width,11788);
  auto configured=std::make_unique<ObservedCompare>(argv[2],width,bytes);
  data.Stage(baseline->bus);data.Stage(configured->bus);assert(baseline->Execute() && configured->Execute());
  std::array<unsigned,32> factored{};
  for(const auto& block:data.blocks)assert(PioneerQ38FactoredQ4Reference::Block(block.original.data(),block.input.data(),factored.data()));
  for(unsigned row=0;row<32;row++){
   assert(baseline->ReadL1(SL::Output+row*4)==data.expected[row] && configured->ReadL1(SL::Output+row*4)==data.expected[row]);
   assert(baseline->ReadL1(SL::Output+128+row*4)==factored[row] && configured->ReadL1(SL::Output+128+row*4)==factored[row]);words+=2;
  }
  assert(baseline->writes[0]==configured->writes[0] && baseline->reads[0]==configured->reads[0]);
  for(unsigned kind=0;kind<2;kind++)assert(baseline->writes[kind]==baseline->reads[kind] && configured->writes[kind]==configured->reads[kind]);
  const unsigned removed=328*(width/256)-22;
  assert(baseline->writes[1]==configured->writes[1]+removed);
  printf("PASS_COMPILED_CONFIGURED K%u exact64 NoC_PACK_FPU_SFPU_originalBytes_independentBothReferences oldPathPairs%u_unchanged newPairs%u_to%u_removed%u;stack%u_touched%u_RV32steps%u_to%u;SYNTHETIC_CLOCK_NOT_CARD_SPEED\n",width,configured->writes[0],baseline->writes[1],configured->writes[1],removed,0x70000-configured->minSP,0x70000-configured->minStore,baseline->steps,configured->steps);
 }
 assert(words==128);static_cast<void>(&NoMore);
}
