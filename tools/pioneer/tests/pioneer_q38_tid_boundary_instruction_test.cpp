/* SPDX-License-Identifier: MIT */
// Execute pinned, emitted RISC quiet helpers with synthetic status inputs.
// No native ownership, completion injection, model or hardware is supplied.
#define PIONEER_Q38_PEER_SOURCE_INSTRUCTION_MACHINE_ONLY
#include "pioneer_q38_peer_source_instruction_test.cpp"
struct TidQuietMachine:Machine {
 const AuditImage& image;std::array<uint32_t,38> status;
 std::map<unsigned,uint32_t> stack;
 std::vector<std::pair<unsigned,uint32_t>> reads;
 unsigned stores=0,minimum=0x70000,changeAfter=UINT32_MAX,changeIndex=0;
 uint32_t changeValue=0;
 static unsigned Address(unsigned i){
  assert(i<38);return i<4?0xffb20040U+i*0x800:i==4?0xffb20064U:i==5?0xffb20068U:0xffb20240U+(i-6)*4;
 }
 TidQuietMachine(const AuditImage& rom,unsigned kind,const std::array<uint32_t,38>& values):image(rom),status(values){
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
  for(unsigned i=0;i<38;i++)if(at==Address(i)){
   const uint32_t value=status[i];reads.emplace_back(at,value);
   if(reads.size()==changeAfter){assert(changeIndex<38);status[changeIndex]=changeValue;}
   return value;
  }
  fprintf(stderr,"UNQUALIFIED TID quiet read%x PC%x\n",at,pc);assert(false);return 0;
 }
 void Store(unsigned at,uint32_t value)override{
  assert(!(at&3) && at>=0x6f000 && at<0x70000); // no counter clear,mode/command/model/host write
  stack[at]=value;stores++;minimum=std::min(minimum,at);
 }
 unsigned Run(bool okay,const std::vector<unsigned>& prefix){
  while(pc!=AuditMachine::Return && pc!=0x60000)Step();
  assert(pc==AuditMachine::Return && regs[10]==unsigned(okay) && regs[1]==AuditMachine::Return && regs[2]==0x70000);
  for(unsigned i:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})assert(regs[i]==0xa0000000U+i);
  assert(reads.size()==prefix.size());
  for(unsigned i=0;i<prefix.size();i++)assert(reads[i].first==Address(prefix[i]));
  const unsigned instructions=steps,writes=stores;const auto observed=reads;
  for(unsigned i=0;i<2048;i++)Step();
  assert(pc==AuditMachine::Return && reads==observed && stores==writes);
  return instructions;
 }
};
static std::vector<unsigned> Sequence(unsigned kind){
 std::vector<unsigned> result={0,1,2,3,4,5};
 if(kind==0){result.push_back(6);result.push_back(22);}
 if(kind==1)for(unsigned i=6;i<38;i++)result.push_back(i);
 return result;
}
static std::pair<bool,std::vector<unsigned>> Expected(unsigned kind,const std::array<uint32_t,38>& status){
 std::vector<unsigned> prefix;
 for(unsigned i:Sequence(kind)){
  prefix.push_back(i);bool bad=i==4?status[i]!=0x10101010U:i<6?status[i]!=0:(status[i]&255)!=0;
  if(bad)return {false,prefix};
 }
 return {true,prefix};
}
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);AuditImage image(argv[1],argv[2]);
 std::array<uint32_t,38> idle={};idle[4]=0x10101010U;
 for(unsigned i=6;i<38;i++)idle[i]=0xa5000000U+i*0x10000U;
 unsigned cases=0,deferred=0;
 for(unsigned kind=0;kind<3;kind++){
  auto positive=std::make_unique<TidQuietMachine>(image,kind,idle);
  unsigned instructions=positive->Run(true,Sequence(kind));cases++;
  printf("PASS emitted TID helper%u/%zu actual ordered4B reads/%u HOST instructions/stack%u;NOT device timing\n",
   kind,Sequence(kind).size(),instructions,0x70000-positive->minimum);
  for(unsigned index=0;index<38;index++)for(unsigned bit=0;bit<32;bit++){
   auto values=idle;values[index]^=1U<<bit;auto expected=Expected(kind,values);
   auto machine=std::make_unique<TidQuietMachine>(image,kind,values);machine->Run(expected.first,expected.second);cases++;
   if(kind==0 && index>=6 && index!=6 && index!=22 && bit<8){
    assert(expected.first);auto full=std::make_unique<TidQuietMachine>(image,1,values);auto guard=Expected(1,values);
    assert(!guard.first);full->Run(false,guard.second);cases++;deferred++;
   }
  }
  const auto sequence=Sequence(kind);
  for(unsigned first=0;first+1<sequence.size();first++)for(bool clear:{false,true}){
   const unsigned index=sequence[first+1];auto values=idle;
   const uint32_t bad=index==4?idle[index]^1U:idle[index]|1U;if(clear)values[index]=bad;
   auto machine=std::make_unique<TidQuietMachine>(image,kind,values);
   machine->changeAfter=first+1;machine->changeIndex=index;machine->changeValue=clear?idle[index]:bad;
   auto effective=values;effective[index]=machine->changeValue;const auto expected=Expected(kind,effective);
   machine->Run(expected.first,expected.second);cases++;
  }
 }
 printf("PASS emitted TID boundary/%u synthetic cases/%u explicit inactive-ID deferrals then full refusal/all38 registers all32bits/"
        "FIFO availability TEST before ANY counter read/overflow+all4initiators/TID0 immediate/high-counter bits ignored/live transitions/"
        "ABI+stack-only writes/2048 inert steps EACH;NO acquisition,model,worker-release,native,physical,Work or TPS proof\n",cases,deferred);
}
