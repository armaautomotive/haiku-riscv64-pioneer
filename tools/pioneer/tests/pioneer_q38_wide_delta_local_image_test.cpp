/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_WIDE_DELTA_INSTRUCTION_TEST 1
#define PIONEER_Q38_WIDE_DELTA_EXPECTED_OBSERVER_TEST 1
#include "pioneer_q38_wide_delta_local_machine.h"
int main(int argc,char** argv)
{
 assert(argc==2);unsigned transitions=0;
 for(unsigned trial=0;trial<4;trial++){
  WideDeltaLocalMachine m(argv[1],trial);auto before=m.memory;
  assert(m.RunWide() && m.Verify());
  assert(m.imports==13 && m.deltaWrites==64 && m.nextWrites==8192 && m.resultWrites==64 && m.csrReads==8320 && m.unit.drained && !m.pendingWords);
  for(const auto& word:before)if(WideDeltaLocalMachine::Range(word.first,W::Prior,32768) || WideDeltaLocalMachine::Range(word.first,W::Query,1288))assert(m.memory.at(word.first)==word.second);
  printf("PASS actual emitted wide-delta trial%u/13imports/8320CSR/ALL8192state+64outputs FP64-order exact/RV32steps%u SFPUops%u stack%u;NOT physical or TPS\n",trial,m.steps,m.unit.ops,0x70000-m.minStack);
 }
 for(unsigned head=0;head<4;head++){
  std::array<uint32_t,8192> state={};
  for(unsigned token=0;token<16;token++){
   WideDeltaLocalMachine m(argv[1],1+(token+head)%3);
   for(unsigned i=0;i<state.size();i++)m.memory[W::Prior+i*4]=state[i];
   assert(m.RunWide() && m.Verify());
   for(unsigned i=0;i<state.size();i++)state[i]=m.memory.at(W::Next+i*4);
   transitions++;
  }
 }
 for(unsigned field=0;field<4;field++)for(unsigned bit=0;bit<32;bit++){
  WideDeltaLocalMachine m(argv[1],1);m.memory[W::Descriptor+field*4]^=1U<<bit;
  assert(!m.RunWide() && !m.configWrites && !m.unit.ops && !m.deltaWrites && !m.nextWrites && !m.resultWrites);
 }
 for(uint32_t at:{uint32_t(W::Prior),uint32_t(W::Prior+32764),uint32_t(W::Query),uint32_t(W::Key+508),uint32_t(W::Value+252),uint32_t(W::Decay),uint32_t(W::Beta)}){
  for(uint32_t bad:{1U,0x80000001U,0x7f800000U,0x7fc00001U,0x7f7fffffU}){
   WideDeltaLocalMachine m(argv[1],1);m.memory[at]=bad;
   assert(!m.RunWide() && !m.configWrites && !m.unit.ops && !m.deltaWrites && !m.nextWrites && !m.resultWrites);
  }
 }
 unsigned lost=0;
 for(unsigned import:{1U,5U,8U,9U,10U,11U,12U,13U}){
  WideDeltaLocalMachine m(argv[1],1);m.failImport=import;bool rejected=false;
  try{rejected=!m.RunWide() || !m.Verify();}catch(const WideDeltaUnit::OwnershipFault&){rejected=true;}
  assert(rejected);lost++;
 }
 for(bool output:{false,true}){
  WideDeltaLocalMachine m(argv[1],1);m.dropStateDrain=!output;m.dropOutputDrain=output;bool rejected=false;
  try{(void)m.RunWide();}catch(const WideDeltaLocalMachine::UnsafeRead&){rejected=true;}
  assert(rejected && !m.resultWrites && m.nextWrites==(output?8192U:0U));
 }
 printf("PASS actual emitted wide-delta/%u nonresetting transitions/all descriptor bits+35 bounded-input refusals/%u missing-imports numerically detected or undefined-row fault/2 missing-drain publication refusals;NEW local layout ONLY,NOT native END,full-model,physical,Work or TPS\n",transitions,lost);
}
