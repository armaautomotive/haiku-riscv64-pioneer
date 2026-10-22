/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RMS_MACHINE_ONLY
#include "pioneer_q38_learned_rms_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
struct RmsFootprintMachine:LearnedRmsMachine {
 unsigned instructionViews=0,loads=0,stores=0,packetViews=0;
 using LearnedRmsMachine::LearnedRmsMachine;
 unsigned Index(uint32_t a)override{assert(PioneerQ38ProjectionGraphSnapshotContains(0,a));instructionViews++;return ScalarMachine::Index(a);}
 uint32_t Load(uint32_t a)override{
  if(!Range(a,0xffb20000,0x2000)){assert(PioneerQ38ProjectionGraphSnapshotContains(0,a));loads++;}return LearnedRmsMachine::Load(a);
 }
 void Store(uint32_t a,uint32_t v)override{
  if(!Range(a,0xffb20000,0x2000)){assert(PioneerQ38ProjectionGraphSnapshotContains(0,a));stores++;}
  if(a==0xffb20040){unsigned local=command[packets>=4?0:3],bytes=command[8];
   for(unsigned i=0;i<bytes;i+=4){assert(PioneerQ38ProjectionGraphSnapshotContains(0,local+i));packetViews++;}}
  LearnedRmsMachine::Store(a,v);
 }
};
int main(int argc,char** argv){
 assert(argc==2);(void)&RmsPlanTests;auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned instructions=0,loads=0,stores=0,packets=0;
 for(unsigned layer:{0U,63U,64U}){
  RmsFootprintMachine m(argv[1],*canonical,layer,layer==64?2:1,6);assert(m.Run());m.Verify();
  instructions+=m.instructionViews;loads+=m.loads;stores+=m.stores;packets+=m.packetViews;
 }
 printf("PASS ACTUAL learned RMS complete QPR1 footprint:instructionViews%u/loads%u/stores%u/NoClocalWords%u;input+learned+normalized scratch/code/stack/trailer/descriptor/summary covered;NOT native mapping/post-launch restore authority\n",instructions,loads,stores,packets);
}
