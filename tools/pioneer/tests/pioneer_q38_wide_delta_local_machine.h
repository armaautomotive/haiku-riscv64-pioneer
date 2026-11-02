/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_WIDE_DELTA_LOCAL_MACHINE_H
#define PIONEER_Q38_WIDE_DELTA_LOCAL_MACHINE_H
#define PIONEER_GRAPH_MATH_MACHINE_ONLY
#include "pioneer_graph_math_image_test.cpp"
#define PIONEER_Q38_DELTA_VECTOR_EXISTING_BITS
#include "pioneer_q38_wide_delta_instruction_model.h"
#include "../PioneerQ38WideDeltaLocalLayout.h"
namespace W=PioneerQ38WideDeltaLocalLayout;
// Fresh TEST processor,not a native owner. Exact NEW import/scratch schedule
// is checked independently of emitted source. Never extends the old owner.
struct WideDeltaLocalMachine:ScalarMachine {
 WideDeltaUnit unit;
 uint32_t cfg[224]={},threads[68]={},ack=0,x0=0,x1=0;
 unsigned imports=0,pendingWords=0,pendingSource=0,pendingRow=0;
 unsigned nextWrites=0,resultWrites=0,deltaWrites=0,csrReads=0,deltaReads=0,configWrites=0;
 unsigned failImport=0;
 bool waiting=false,dropStateDrain=false,dropOutputDrain=false;
 struct UnsafeRead{};
 WideDeltaLocalMachine(const char* path,unsigned trial):ScalarMachine(path,64,0){
  unit.localImage=true;memory.clear();
  memory[W::Descriptor]=W::Magic;memory[W::Descriptor+4]=128;memory[W::Descriptor+8]=64;memory[W::Descriptor+12]=0;
  for(unsigned i=0;i<8192;i++){memory[W::Prior+i*4]=Bits(trial?float(sin(i*.031+trial*.17)*.003):0);memory[W::Next+i*4]=0xdeadbeef;}
  for(unsigned i=0;i<128;i++){
   memory[W::Query+i*4]=Bits(trial?float(sin(i*.123+trial*.317)*.01):0);
   memory[W::Key+i*4]=Bits(trial?float(cos(i*.153+trial*.217)*.03):0);
  }
  for(unsigned i=0;i<64;i++){
   memory[W::Value+i*4]=Bits(trial?float(sin(i*.013+trial*.117)*2.1):0);
   memory[W::Delta+i*4]=memory[W::Output+i*4]=0xdeadbeef;
  }
  memory[W::Decay]=Bits(trial==2?0:trial==3?1:.83f);memory[W::Beta]=Bits(trial==2?1:trial==3?0:.37f);
  memory[0x17ffc0]=memory[0x17ffc4]=memory[0x17ffc8]=0;
 }
 unsigned Index(uint32_t at) override {
  if(at==0x6000)return 0;
  assert(Range(at,W::Code,unsigned(code.size())) && !(at&3));return 1+(at-W::Code)/4;
 }
 bool CodeAddress(uint32_t at)const override{return at==0x6000 || Range(at,W::Code,unsigned(code.size()));}
 void Instruction(uint32_t op) override {
  const unsigned code=op>>24;
  if(code==0xb2){assert(!pendingWords);const unsigned i=(op>>16)&255;assert(i<68);threads[i]=op&65535;return;}
  if(op==0xa2101816){assert(!waiting && !ack);waiting=true;return;}
  if(op==0x4500017e){
   assert(waiting);waiting=false;
   if(pendingWords){
    if(imports!=failImport){
     for(unsigned i=0;i<pendingWords;i++)unit.state[pendingRow*16+i]=memory.at(pendingSource+i*4);
     for(unsigned r=pendingRow;r<pendingRow+pendingWords/16;r++)unit.defined[r]=true;
    }
    pendingWords=0;
   }
   const bool dropped=(dropStateDrain && imports==13 && unit.stores==8256 && !nextWrites)
    || (dropOutputDrain && unit.stores==8320);
   if(!dropped)unit.Drain();ack=1;return;
  }
  if((op&0xfff00000)==0x5e200000){assert(!pendingWords);x0=op&1023;x1=(op>>10)&1023;return;}
  if(op==0x5120000a || op==0x5420000f){assert(!pendingWords);return;}
  if(op==0x42000081){
   assert(!pendingWords && !threads[5] && cfg[0]==0x4231 && cfg[1]==0xa0200000 && !cfg[3] && !cfg[49]
    && cfg[64]==0x100010 && cfg[65]==16 && !cfg[72] && cfg[73]==0x13 && !x0);
   const unsigned source=(cfg[76]+1)*16,row=(cfg[84]&65535)/16-4,words=x1+1;
   assert((cfg[84]&0xffff0000)==0x400000 && row<512 && !(words%16) && words<=8192-row*16);
   if(imports<8)assert(source==W::Prior+imports*4096 && row==imports*64 && words==1024);
   else if(imports==8 || imports==10)assert(source==W::Value && !row && words==64);
   else if(imports==9 || imports==12)assert(source==W::Prior && !row && words==64);
   else {assert(imports==11 && source==W::Delta && !row && words==64 && deltaWrites==64);}
   imports++;pendingSource=source;pendingRow=row;pendingWords=words;unit.drained=false;return;
  }
  assert(!pendingWords && imports>=8 && threads[2]==1 && threads[3]==1 && threads[5]==4 && cfg[0]==0x4231 && cfg[1]==0xa0200000);
  if(code==0x70 && ((op>>20)&15))assert(imports==12 && unit.stores==64 &&
   (((op>>20)&15)==6 || ((op>>20)&15)==7) && !(op&0x3fc));
  if(code==0x72){
   const unsigned at=op&1023,reg=(op>>20)&15;
   if(unit.stores<64)assert((imports==9 || imports==11) && reg==0 && at==(unit.stores/32)*2);
   else if(unit.stores<8256)assert(imports==13 && reg==0 && at==((unit.stores-64)/64)*4+(((unit.stores-64)%64)?2:0));
   else assert(imports==13 && nextWrites==8192 && reg==6 && at==((unit.stores-8256)/32)*2 && unit.stores<8320);
  }
  unit.Op(op);
 }
 uint32_t Load(uint32_t at) override {
  assert(!(at&3));if(Range(at,W::Code,unsigned(code.size())))return l1[Index(at)];
  if(at==0xffe000fc)return ack;
  if(Range(at,0xffef0000,224*4))return cfg[(at-0xffef0000)/4];
  if(Range(at,0xffbd8000,32768)){
   if(!unit.drained || pendingWords)throw UnsafeRead{};
   assert(!cfg[3]);const unsigned index=(at-0xffbd8000)/4;
   if(imports==9 || imports==11){
    assert(index==(deltaReads%32)*2+(imports==11?1:0) && unit.stores==(imports==9?32U:64U));deltaReads++;
   }else if(nextWrites<8192)assert(imports==13 && unit.stores==8256 && index==nextWrites && deltaWrites==64);
   else assert(imports==13 && unit.stores==8320 && index==resultWrites);
   csrReads++;return unit.state[index];
  }
  assert(Range(at,W::Descriptor,16) || Range(at,0x6e000,8192) || Range(at,W::Prior,32768)
   || Range(at,W::Query,1288) || Range(at,0x17ffc0,12));return memory.at(at);
 }
 void Store(uint32_t at,uint32_t value) override {
  assert(!(at&3));if(at==0xffe000fc){assert(!value);ack=0;return;}
  if(at==0xffe40000){Instruction(value);return;}
  if(Range(at,0xffef0000,224*4)){
   assert(unit.drained && !pendingWords);const unsigned i=(at-0xffef0000)/4;
   const unsigned allowed[]={0,1,2,3,6,7,220,20,24,28,49,50,56,59,64,65,72,73,76,84,86,112,113,120,121,124};
   bool found=false;for(auto a:allowed)found|=i==a;assert(found);cfg[i]=value;configWrites++;return;
  }
  assert(Range(at,0x6e000,8192) || Range(at,W::Delta,256) || Range(at,W::Next,32768) || Range(at,W::Output,256) || Range(at,0x17ffc0,12));
  if(Range(at,0x6e000,8192))minStack=std::min(minStack,unsigned(at));
  if(Range(at,W::Delta,256)){
   assert(unit.drained && deltaReads==deltaWrites+1 && at==W::Delta+((deltaWrites%32)*2+(deltaWrites>=32))*4);deltaWrites++;
  }
  if(Range(at,W::Next,32768)){assert(unit.drained && imports==13 && csrReads==64+nextWrites+1 && at==W::Next+nextWrites*4);nextWrites++;}
  if(Range(at,W::Output,256)){assert(unit.drained && nextWrites==8192 && at==W::Output+resultWrites*4);resultWrites++;}
  memory[at]=value;
 }
 bool RunWide(){
  while(pc!=W::Park && pc!=W::FaultPark)StepScalar();
  if(pc==W::FaultPark){assert(memory.at(0x17ffc8)==W::Fault);return false;}
  assert(memory.at(0x17ffc8)==W::Success && minStack>=0x6e000);return memory.at(0x17ffc4)==64;
 }
#ifdef PIONEER_Q38_WIDE_DELTA_EXPECTED_OBSERVER_TEST
 bool Verify(){
  // Independent scalar FP64-order reduction and non-fused FP32 update.
  // Observer ONLY;these numbers never enter the instruction processor.
  std::array<float,8192> next={};double sums[64]={};bool same=true;
  const float decay=Float(memory.at(W::Decay)),beta=Float(memory.at(W::Beta));
  for(unsigned v=0;v<64;v++){
   double prediction=0;
   for(unsigned key=0;key<128;key++)prediction+=double(Float(memory.at(W::Prior+(key*64+v)*4))*decay)*double(Float(memory.at(W::Key+key*4)));
   const float delta=(Float(memory.at(W::Value+v*4))-float(prediction))*beta;
   for(unsigned key=0;key<128;key++){
    const unsigned at=key*64+v;
    next[at]=Float(memory.at(W::Prior+at*4))*decay+Float(memory.at(W::Key+key*4))*delta;
    same&=memory.at(W::Next+at*4)==Bits(next[at]);
    sums[v]+=double(next[at])*double(Float(memory.at(W::Query+key*4)));
   }
   same&=memory.at(W::Output+v*4)==Bits(float(sums[v])*Float(0x3db504f3));
  }
  return same;
 }
#endif
};
#endif
