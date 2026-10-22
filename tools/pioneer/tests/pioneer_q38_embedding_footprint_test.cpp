/* SPDX-License-Identifier: MIT */
// Actual QPE1 L1 traffic must fit the COMPLETE future projection snapshot.
// This is footprint DATA evidence,NOT safe post-launch restoration proof.
#define PIONEER_Q38_EMBEDDING_MACHINE_ONLY
#include "pioneer_q38_embedding_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
struct EmbeddingFootprintMachine:EmbeddingMachine {
 unsigned instructionViews=0,loads=0,stores=0,packetViews=0;
 using EmbeddingMachine::EmbeddingMachine;
 unsigned Index(uint32_t a)override{
  assert(PioneerQ38ProjectionGraphSnapshotContains(0,a));instructionViews++;
  return ScalarMachine::Index(a);
 }
 uint32_t Load(uint32_t a)override{
  if(!Range(a,0xffb20000,0x2000)){assert(PioneerQ38ProjectionGraphSnapshotContains(0,a));loads++;}
  return EmbeddingMachine::Load(a);
 }
 void Store(uint32_t a,uint32_t v)override{
  if(!Range(a,0xffb20000,0x2000)){assert(PioneerQ38ProjectionGraphSnapshotContains(0,a));stores++;}
  if(a==0xffb20040){
   bool write=packets>=20;unsigned local=command[write?0:3],bytes=command[8];
   for(unsigned i=0;i<bytes;i+=4){assert(PioneerQ38ProjectionGraphSnapshotContains(0,local+i));packetViews++;}
  }
  EmbeddingMachine::Store(a,v);
 }
};
int main(int argc,char** argv){
 assert(argc==2);(void)&PlanTests;auto canonical=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(canonical->Build());
 unsigned views=0,loads=0,stores=0,packets=0;
 for(unsigned token:{0U,35519U,35520U,248319U}){
  EmbeddingFootprintMachine m(argv[1],*canonical,token,8191,6);assert(m.Run());m.Verify();
  views+=m.instructionViews;loads+=m.loads;stores+=m.stores;packets+=m.packetViews;
 }
 printf("PASS actual QPE1 within COMPLETE QPR1 L1 footprint:instructionViews%u/loads%u/stores%u/NoC-local-words%u;4 boundary tokens,full5120 output,descriptor/summary/stack/trailer/code/tiles covered;NO_NATIVE_MAPPING_OR_POST_LAUNCH_RESTORE_AUTHORITY\n",views,loads,stores,packets);
}
