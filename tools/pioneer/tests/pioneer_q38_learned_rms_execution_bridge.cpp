/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RMS_MACHINE_ONLY
#include "pioneer_q38_learned_rms_image_test.cpp"
#include "pioneer_q38_learned_rms_execution_bridge.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingFootprint.h"
void ExecuteLearnedRmsBridge(const char* image,LearnedRmsExecutionBridge& t){
 (void)&RmsPlanTests;assert(t.l1.size()==73737);auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 auto at=[](unsigned address){unsigned n=0;for(unsigned i=0;i<6;i++){auto s=PioneerQ38EmbeddingSnapshotSpan(i);
  if(address>=s.address && address-s.address<s.words*4)return n+(address-s.address)/4;n+=s.words;}assert(false);return 0U;};
 LearnedRmsMachine m(image,*plan,0,0,t.l1[at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+12)]);
 for(unsigned i=0;i<73737;i++){
  unsigned a=PioneerQ38EmbeddingSnapshotAddress(i);m.memory[a]=t.l1[i];if(m.CodeAddress(a))m.l1[m.Index(a)]=t.l1[i];
 }
 for(unsigned i=0;i<64;i++)m.descriptor[i]=t.l1[at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4)];
 for(unsigned i=0;i<16;i++)m.source[i]=t.source[i];
 for(unsigned i=0;i<10;i++)m.command[i]=t.command[i];
 for(unsigned i=0;i<8;i++)m.counts[i]=m.before[i]=t.counts[i];
 m.input=m.resident=t.resident;m.backup=t.backup;
 m.failPacket=t.failPacket;m.badCounter=t.badCounter;m.sourceFault=t.sourceFault;m.descriptorFault=t.descriptorFault;
 // Execute EXACT installed bytes. No host result is supplied or fixed up.
 while(m.pc!=0x64038 && m.pc!=0x64058)m.StepScalar();
 if(m.pc==0x64038)m.Verify(); // independent double/sqrtf TEST observer,never used as device input
 for(unsigned i=0;i<73737;i++){auto a=PioneerQ38EmbeddingSnapshotAddress(i);t.l1[i]=m.CodeAddress(a)?m.l1[m.Index(a)]:m.memory.at(a);}
 for(unsigned i=0;i<10;i++)t.command[i]=m.command[i];
 for(unsigned i=0;i<8;i++)t.counts[i]=m.counts[i];
 for(unsigned i=0;i<16;i++)t.source[i]=m.source[i];
 t.resident=m.resident;t.backup=m.backup;t.steps=m.steps;t.packets=m.packets;t.pc=m.pc;
}
