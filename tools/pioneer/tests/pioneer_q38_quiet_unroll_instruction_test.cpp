/* SPDX-License-Identifier: MIT */
// Execute actual emitted Quiet functions. Synthetic status-register inputs
// only:these calls do NOT acquire native ownership or execute a model.
#define PIONEER_Q38_PEER_SOURCE_INSTRUCTION_MACHINE_ONLY
#include "pioneer_q38_peer_source_instruction_test.cpp"
struct QuietMachine:Machine {
 const AuditImage& image;std::array<uint32_t,36> status;
 std::map<unsigned,uint32_t> stack;
 std::vector<std::pair<unsigned,uint32_t>> reads;
 unsigned stores=0,minimum=0x70000;
 unsigned changeAfter=UINT32_MAX,changeIndex=0;uint32_t changeValue=0;
 static unsigned Address(unsigned index){assert(index<36);return index<4?0xffb20040U+index*0x800:0xffb20240U+(index-4)*4;}
 QuietMachine(const AuditImage& rom,unsigned kind,const std::array<uint32_t,36>& values):image(rom),status(values){
  regs[1]=AuditMachine::Return;regs[2]=0x70000;
  for(unsigned i:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})regs[i]=0xa0000000U+i;
  pc=image.entry[kind];
 }
 unsigned StepLimit()const override{return 100000;}
 bool CodeAddress(unsigned at)const override{return (at>=0x40000 && at-0x40000<image.text.size()) || (at>=0x60000 && at<0x60008);}
 unsigned Index(unsigned at)override{
  assert(CodeAddress(at) && !(at&3));l1[8180]=at<0x60000?Word(image.text,at-0x40000):Word(image.terminal,at-0x60000);return 8180;
 }
 uint32_t Load(unsigned at)override{
  assert(!(at&3));if(CodeAddress(at))return l1[Index(at)];
  if(at>=0x6f000 && at<0x70000){assert(stack.count(at));return stack.at(at);}
  for(unsigned i=0;i<36;i++)if(at==Address(i)){
   const auto value=status[i];reads.emplace_back(at,value);
   if(reads.size()==changeAfter){assert(changeIndex<36);status[changeIndex]=changeValue;}
   return value;
  }
  fprintf(stderr,"UNQUALIFIED Quiet read%x PC%x\n",at,pc);assert(false);return 0;
 }
 void Store(unsigned at,uint32_t value)override{
  assert(!(at&3) && at>=0x6f000 && at<0x70000); // NEVER NoC command,CSR,model or host writes
  stack[at]=value;stores++;minimum=std::min(minimum,at);
 }
 unsigned Run(bool expected,unsigned prefix){
  while(pc!=AuditMachine::Return && pc!=0x60000)Step();
  assert(pc==AuditMachine::Return && regs[10]==unsigned(expected) && regs[1]==AuditMachine::Return && regs[2]==0x70000);
  for(unsigned i:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})assert(regs[i]==0xa0000000U+i);
  assert(reads.size()==prefix);for(unsigned i=0;i<prefix;i++){
   assert(reads[i].first==Address(i));if(changeAfter==UINT32_MAX)assert(reads[i].second==status[i]);
  }
  const unsigned instructions=steps,writes=stores;const auto observed=reads;
  for(unsigned i=0;i<2048;i++)Step();
  assert(pc==AuditMachine::Return && reads==observed && stores==writes);
  return instructions;
 }
};
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);AuditImage before(argv[1],argv[3]),after(argv[2],argv[4]);
 unsigned cases=0;std::array<uint32_t,36> idle={};
 for(unsigned i=4;i<36;i++)idle[i]=0xa5000000U+i*0x10000U; // high queue bits MUST remain ignored
 for(unsigned kind=0;kind<3;kind++){
  auto a=std::make_unique<QuietMachine>(before,kind,idle),b=std::make_unique<QuietMachine>(after,kind,idle);
  const unsigned oldSteps=a->Run(true,36),newSteps=b->Run(true,36);
  assert(a->reads==b->reads && newSteps<oldSteps && b->minimum>=a->minimum);cases++;
  printf("PASS emitted Quiet%u/all36 SAME ordered4B status reads+values/NO traffic or non-stack writes/instructions%u->%u/stack%u/%u;NOT physical timing or model/TPS proof\n",
   kind,oldSteps,newSteps,0x70000-a->minimum,0x70000-b->minimum);
  for(unsigned index=0;index<36;index++)for(unsigned bit=0;bit<32;bit++){
   auto values=idle;values[index]^=1U<<bit;
   const bool okay=index>=4 && bit>=8;const unsigned prefix=okay?36:index+1;
   auto x=std::make_unique<QuietMachine>(before,kind,values),y=std::make_unique<QuietMachine>(after,kind,values);
   x->Run(okay,prefix);y->Run(okay,prefix);assert(x->reads==y->reads);cases++;
  }
  for(unsigned first=0;first<36;first++){
   auto values=idle;values[first]|=1;values[35]|=0x80;
   auto x=std::make_unique<QuietMachine>(before,kind,values),y=std::make_unique<QuietMachine>(after,kind,values);
   x->Run(false,first+1);y->Run(false,first+1);assert(x->reads==y->reads);cases++;
  }
  for(unsigned first=0;first<35;first++)for(bool clear:{false,true}){
   auto values=idle;if(clear)values[first+1]|=1;
   auto x=std::make_unique<QuietMachine>(before,kind,values),y=std::make_unique<QuietMachine>(after,kind,values);
   for(auto* machine:{x.get(),y.get()}){
    machine->changeAfter=first+1;machine->changeIndex=first+1;machine->changeValue=clear?idle[first+1]:idle[first+1]|1;
   }
   x->Run(clear,clear?36:first+2);y->Run(clear,clear?36:first+2);assert(x->reads==y->reads);cases++;
  }
 }
 printf("PASS emitted queue-check equivalence/%u paired synthetic cases/all32bits at all36 registers/early-exit prefixes+ignored high queue bits+multiple faults+live pending/clear transitions/ABI preserved/actual2048 inert instructions each;NO cached/skipped/wider MMIO,model/native/physical/Work/TPS qualification\n",cases);
}
