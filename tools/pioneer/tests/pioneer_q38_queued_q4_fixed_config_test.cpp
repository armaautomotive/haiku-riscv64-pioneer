/* SPDX-License-Identifier: MIT */
// Fixed-program transaction component only,NO device/native/speed grant.
#define PIONEER_Q38_QUEUED_Q4_FIXED_CONFIG_IMAGE 1
#define PIONEER_Q38_QUEUED_Q4_WORD_MODEL_ONLY 1
#include "pioneer_q38_queued_q4_block_words_test.cpp"
#include "../PioneerQ38QueuedQ4FixedConfig.h"
using F=PioneerQ38QueuedQ4FixedConfig;
static const unsigned fields[]={50,64,72,76,84,124};
static const unsigned initial[]={0x181,0x01000017,0x27,0x980f,0x00400040,0x961f};
static const unsigned finalWords[]={0x17f,0x0100001f,15,0x9cab,0x00400140,0xafff};
static bool Begin(F& f,Observer& e){return f.Begin(e,initial[5],initial[3],finalWords[5],finalWords[3]);}
static void Finish(F& f,Observer& e){for(unsigned i=0;i<6;i++)assert(f.Config(e,fields[i],finalWords[i]));}
int main(){
 unsigned cases=0,badFields=0,badHigh=0,badBegin=0,faults=0;
 for(unsigned field=0;field<6;field++)for(unsigned byte=0;byte<2;byte++)for(unsigned value=0;value<256;value++){
  F f;Q old;Observer a,b;a.config.fill(0xa55ac33c);b.config=a.config;
  assert(!f.Config(b,50,0) && !f.Verify(b) && b.words.empty() && !b.reads);
  assert(old.Begin() && Begin(f,b) && b.words.size()==24);
  for(unsigned i=0;i<6;i++)assert(old.Config(a,fields[i],initial[i]));
  assert(a.config==b.config);const auto seeded=b.words;
  assert(!Begin(f,b) && b.words==seeded);
  const unsigned target=initial[field]^(value<<(byte*8));
  const auto before=b.words.size();
  assert(old.Config(a,fields[field],target) && f.Config(b,fields[field],target));
  assert(b.words.size()==before+2 && a.config==b.config);
  for(unsigned i=0;i<6;i++)assert(old.Config(a,fields[i],finalWords[i]));
  Finish(f,b);assert(a.config==b.config && old.Verify(a) && f.Verify(b) && b.reads==6 && !f.seen && f.phase==F::Idle);
  // Completed previous state is NOT authority in a new block.
  for(auto& word:b.config)word^=0xf18a5c27U;
  const auto fresh=b.words.size();assert(Begin(f,b) && b.words.size()==fresh+24);
  for(unsigned i=0;i<6;i++)assert(b.config[fields[i]]==initial[i]);
  Finish(f,b);assert(f.Verify(b));cases++;
 }
 for(unsigned index=0;index<256;index++){
  bool known=false;for(unsigned field:fields)known|=field==index;if(known)continue;
  F f;Observer e;assert(Begin(f,e));const auto words=e.words;
  assert(!f.Config(e,index,0) && f.phase==F::Failed && e.words==words && !f.Verify(e) && !e.reads);
  assert(!Begin(f,e) && !f.Config(e,50,0) && e.words==words);badFields++;
 }
 for(unsigned field=0;field<6;field++)for(unsigned bit=16;bit<32;bit++){
  F f;Observer e;assert(Begin(f,e));const auto words=e.words;
  assert(!f.Config(e,fields[field],initial[field]^(1U<<bit)) && f.phase==F::Failed && e.words==words);
  assert(!f.Verify(e) && !Begin(f,e) && !e.reads);badHigh++;
 }
 for(unsigned argument=0;argument<4;argument++)for(unsigned bit=16;bit<32;bit++){
  unsigned args[]={initial[5],initial[3],finalWords[5],finalWords[3]};args[argument]|=1U<<bit;
  F f;Observer e;assert(!f.Begin(e,args[0],args[1],args[2],args[3]) && f.phase==F::Failed && e.words.empty());badBegin++;
 }
 {F f;Observer e;assert(!f.Begin(e,2,1,1,1) && f.phase==F::Failed && e.words.empty());badBegin++;}
 for(unsigned at=0;at<6;at++)for(bool lost:{false,true}){
  F f;Observer e;assert(Begin(f,e));Finish(f,e);e.bad=at;e.lost=lost;
  assert(!f.Verify(e) && f.phase==F::Failed && e.reads==at+1);
  const auto words=e.words;assert(!f.Verify(e) && !Begin(f,e) && !f.Config(e,50,0) && e.words==words && e.reads==at+1);faults++;
 }
 {F f;Observer e;assert(Begin(f,e) && !f.Verify(e) && f.phase==F::Failed);}
 printf("PASS_FIXED_CONFIG_TRANSACTION cases%u badFields%u highHalfRefusals%u beginRefusals%u finalReadFaults%u sixFullSeedsEveryBlock sixRealFinalReads unchangedBackendWait NO_COMPLETE_WORKER_NATIVE_DEVICE_OR_TPS\n",cases,badFields,badHigh,badBegin,faults);
}
