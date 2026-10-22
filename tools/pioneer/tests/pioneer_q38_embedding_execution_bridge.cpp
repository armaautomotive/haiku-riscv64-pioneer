/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_EMBEDDING_MACHINE_ONLY
#include "pioneer_q38_embedding_image_test.cpp"
#include "pioneer_q38_embedding_execution_bridge.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingFootprint.h"
void ExecuteEmbeddingBridge(const char* image,EmbeddingExecutionBridge& t){
 (void)&PlanTests;assert(t.l1.size()==73737);auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 auto at=[](unsigned address){unsigned n=0;for(unsigned i=0;i<6;i++){auto s=PioneerQ38EmbeddingSnapshotSpan(i);
  if(address>=s.address && address-s.address<s.words*4)return n+(address-s.address)/4;n+=s.words;}assert(false);return 0U;};
 EmbeddingMachine m(image,*plan,t.l1[at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+20)],t.l1[at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+160)],t.l1[at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+12)]);
 if(t.boundedEmbedding)for(unsigned block=0;block<20;block++)for(unsigned lane=0;lane<16;lane++){
  unsigned token=t.l1[at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+20)];
  uint16_t d=uint16_t(0x1800+((block+lane+token%7)%7)*32),n=uint16_t(0x1000+((block*3+lane+token%5)%5)*64);
  m.Put(block,lane,0,uint8_t(d));m.Put(block,lane,1,uint8_t(d>>8));
  m.Put(block,lane,2,uint8_t(n));m.Put(block,lane,3,uint8_t(n>>8));
 }
 for(unsigned i=0;i<73737;i++){
  unsigned address=PioneerQ38EmbeddingSnapshotAddress(i);m.memory[address]=t.l1[i];if(m.CodeAddress(address))m.l1[m.Index(address)]=t.l1[i];
 }
 for(unsigned i=0;i<64;i++)m.descriptor[i]=t.l1[at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4)];
 for(unsigned i=0;i<16;i++)m.seals[i]=t.source[i];
 for(unsigned i=0;i<10;i++)m.command[i]=t.command[i];
 for(unsigned i=0;i<8;i++)m.counts[i]=m.before[i]=t.counts[i];
 m.resident=t.resident;m.failPacket=t.failPacket;
 // Legacy local test guards may contain arbitrary original values here;
 // do not invoke its fixture-only Run guards. Execute the SAME real bytes.
 while(m.pc!=0x64038 && m.pc!=0x64058)m.StepScalar();
 if(t.boundedEmbedding && m.pc==0x64038)m.Verify(); // independent original-Q4_K TEST observer ONLY
 for(unsigned i=0;i<73737;i++){auto a=PioneerQ38EmbeddingSnapshotAddress(i);t.l1[i]=m.CodeAddress(a)?m.l1[m.Index(a)]:m.memory.at(a);}
 for(unsigned i=0;i<10;i++)t.command[i]=m.command[i];for(unsigned i=0;i<8;i++)t.counts[i]=m.counts[i];
 t.resident=m.resident;t.steps=m.steps;t.packets=m.packets;t.pc=m.pc;
}
