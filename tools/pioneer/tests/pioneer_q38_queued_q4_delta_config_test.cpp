/* SPDX-License-Identifier: MIT */
// OFFBOARD transaction equivalence; no native/device/performance grant.
#define PIONEER_Q38_QUEUED_Q4_DELTA_CONFIG_IMAGE 1
#define PIONEER_Q38_QUEUED_Q4_WORD_MODEL_ONLY 1
#include "pioneer_q38_queued_q4_block_words_test.cpp"
#include "../PioneerQ38QueuedQ4DeltaConfig.h"
using D=PioneerQ38QueuedQ4DeltaConfig;
int main(){
 const unsigned indices[]={1,50,64,65,72,73,76,84,86,124};
 unsigned cases=0,refused=0,faults=0;uint64_t saved=0;
 for(unsigned index:indices)for(unsigned byte=0;byte<4;byte++)for(unsigned value=0;value<256;value++){
  Q old;D next;Observer a,b;a.config.fill(0xa55ac33c);b.config=a.config;
  const unsigned seed=0x9124e53aU^(index*0x1020408U),target=seed^(value<<(byte*8));
  assert(old.Begin() && next.Begin() && !next.Begin());
  assert(old.Config(a,index,seed) && next.Config(b,index,seed) && b.words.size()==4 && a.config==b.config);
  const unsigned before=unsigned(b.words.size());
  assert(old.Config(a,index,target) && next.Config(b,index,target) && a.config==b.config);
  assert(b.words.size()-before==unsigned(value!=0));
  const unsigned same=unsigned(b.words.size());
  assert(old.Config(a,index,target) && next.Config(b,index,target) && a.config==b.config && b.words.size()==same);
  assert(old.Verify(a) && next.Verify(b) && b.reads==1 && next.phase==Q::Idle && !next.seen);
  saved+=a.words.size()-b.words.size();
  // Old expected[] contents are NOT authority for a different block.
  // Simulate a changed hardware word;first field access rewrites ALL4.
  a.config[index]^=0xf18a5c27U;b.config[index]=a.config[index];
  assert(old.Begin() && next.Begin());const unsigned fresh=unsigned(b.words.size());
  assert(old.Config(a,index,target) && next.Config(b,index,target) && b.words.size()==fresh+4 && a.config==b.config);
  assert(old.Verify(a) && next.Verify(b) && !next.Verify(b));cases++;
 }
 for(unsigned index=0;index<256;index++)if(D::Slot(index)==D::Fields){
  D d;Observer e;assert(d.Begin() && !d.Config(e,index,0) && d.phase==D::Failed && e.words.empty());
  assert(!d.Verify(e) && !d.Begin() && !d.Config(e,1,0) && !e.reads);refused++;
 }
 for(unsigned at=0;at<D::Fields;at++)for(bool lost:{false,true}){
  D d;Observer e;assert(d.Begin());for(unsigned index:indices){assert(d.Config(e,index,index*0x1020408U));assert(d.Config(e,index,index*0x1020408U));}
  e.bad=at;e.lost=lost;assert(!d.Verify(e) && e.reads==at+1 && d.phase==D::Failed);
  const auto words=e.words;assert(!d.Verify(e) && !d.Begin() && !d.Config(e,1,0) && e.words==words && e.reads==at+1);faults++;
 }
 printf("PASS_DELTA_CONFIG_TRANSACTION cases%u badFields%u finalReadFaults%u omittedWords%llu fullFirstWriteEveryBlock unchangedBackendWaitAndVerify NO_COMPLETE_WORKER_NATIVE_DEVICE_OR_TPS\n",cases,refused,faults,(unsigned long long)saved);
}
