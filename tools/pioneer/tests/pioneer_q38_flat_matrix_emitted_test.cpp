/* SPDX-License-Identifier: MIT */
// Synthetic ABI arguments ONLY;actual old/new frozen RV32 predicate bodies.
// No acquired owner,model products,NoC endpoint,physical time or completion.
#define main pioneer_flat_data_fixture_unused_main
#include "pioneer_q38_flat_matrix_grammar_test.cpp"
#undef main
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include <fstream>
#include <memory>
#include <string>
struct GrammarRom {
 std::vector<uint8_t> text,aux;unsigned entries[2]={};
 GrammarRom(const char* dir,const char* mapping):text(Read((std::string(dir)+"/text.bin").c_str())),
  aux(Read((std::string(dir)+"/conv.bin").c_str())){
  std::ifstream in(mapping);for(auto& value:entries){in>>std::hex>>value;assert(in.good() && value>=0x40000 && value<0x60000);}
 }
 bool Contains(unsigned a)const{return (a>=0x40000 && a-0x40000+4<=text.size()) || (a>=0x64000 && a-0x64000+4<=aux.size());}
 unsigned At(unsigned a)const{assert(!(a&3) && Contains(a));return a<0x60000?Word(text,a-0x40000):Word(aux,a-0x64000);}
};
struct GrammarMachine:Machine {
 const GrammarRom& rom;Words boot,job;std::array<uint32_t,2048> stack={};
 enum {Boot=0x80000,Job=0x80100,Return=0x60004,Stack=0x6e000,Top=0x70000};
 unsigned stores=0;
 GrammarMachine(const GrammarRom& image,unsigned kind,const Words& b,const Words& j,
                unsigned previous,unsigned phase,unsigned bootArg=Boot,unsigned jobArg=Job):rom(image),boot(b),job(j){
  regs[1]=Return;regs[2]=Top;
  for(unsigned i:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})regs[i]=0x17ac0000U+i;
  regs[10]=kind?jobArg:bootArg;regs[11]=bootArg;regs[12]=previous;regs[13]=phase;pc=rom.entries[kind];
 }
 bool CodeAddress(unsigned address)const override{return rom.Contains(address);}
 unsigned Index(unsigned address)override{l1[8180]=rom.At(address);return 8180;}
 unsigned StepLimit()const override{return 20000;}
 void Instruction(uint32_t)override{assert(false && "integer grammar reached Tensix instruction");}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));
  if(rom.Contains(a))return rom.At(a);
  if(a>=Boot && a<Boot+256)return boot[(a-Boot)/4];
  if(a>=Job && a<Job+256)return job[(a-Job)/4];
  if(a>=Stack && a<Top)return stack[(a-Stack)/4];
  fprintf(stderr,"UNMAPPED grammar LOAD%08x pc%08x\n",a,pc);assert(false);return 0;
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3) && a>=Stack && a<Top);stack[(a-Stack)/4]=value;stores++;
 }
 bool Run(){
  const auto beforeBoot=boot,beforeJob=job;
  while(pc!=Return)Step();
  assert(regs[10]<=1 && regs[1]==Return && regs[2]==Top && boot==beforeBoot && job==beforeJob);
  for(unsigned i:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U,27U})assert(regs[i]==0x17ac0000U+i);
  assert(!matrixInstructions && !configWrites && !inputWrites && !vectorWrites && !outputReads);
  return regs[10];
 }
};
struct GrammarComparisons {
 GrammarRom old,next;uint64_t cases=0,yes=0,no=0;
 GrammarComparisons(char** a):old(a[1],a[3]),next(a[2],a[4]){}
 std::pair<unsigned,unsigned> Check(unsigned kind,const Words& b,const Words& j,unsigned previous=0,unsigned phase=0,
   unsigned bp=GrammarMachine::Boot,unsigned jp=GrammarMachine::Job){
  auto a=std::make_unique<GrammarMachine>(old,kind,b,j,previous,phase,bp,jp);
  auto c=std::make_unique<GrammarMachine>(next,kind,b,j,previous,phase,bp,jp);
  const uint32_t* boot=bp?b.data():nullptr;
  const uint32_t* job=jp==GrammarMachine::Boot?boot:jp?j.data():nullptr;
  const bool expected=kind?Old::JobShape(job,boot,previous,phase):Old::BootShape(boot);
  const bool first=a->Run(),second=c->Run();
  if(first!=expected || second!=expected){fprintf(stderr,"EMITTED grammar case%llu/kind%u/op%u/expected%u/actual%u,%u\n",
   (unsigned long long)cases,kind,j[2],expected,first,second);assert(false);}
  cases++;if(expected)yes++;else no++;
  return {a->steps,c->steps};
 }
};
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);GrammarComparisons tests(argv);
 for(unsigned worker=1;worker<=119;worker++){
  auto [b,j]=Seed(worker);tests.Check(0,b,j);
  for(unsigned op=1;op<=12;op++){
   auto [boot,job]=Operation(worker,op);tests.Check(1,boot,job,0,op>=9 && op<=11?op-9:0);
  }
 }
 auto [baseBoot,baseJob]=Seed(1);
 const auto bootSteps=tests.Check(0,baseBoot,baseJob),jobSteps=tests.Check(1,baseBoot,baseJob);
 assert(bootSteps.second<bootSteps.first && jobSteps.second<jobSteps.first);
 printf("EMITTED_SYNTHETIC_ARGUMENT_HOST_INSTRUCTIONS boot%u->%u ordinaryJob%u->%u;NOT device cycles,TPS or sustained model time\n",
  bootSteps.first,bootSteps.second,jobSteps.first,jobSteps.second);
 for(unsigned field=0;field<64;field++)for(unsigned bit=0;bit<32;bit++){
  auto b=baseBoot;b[field]^=uint32_t(1)<<bit;tests.Check(0,b,baseJob);
 }
 for(unsigned op:{1U,2U,3U,6U,9U,10U,11U,12U}){
  auto [boot,job]=Operation(1,op);const unsigned phase=op>=9 && op<=11?op-9:0;
  for(unsigned field=0;field<128;field++)for(unsigned bit=0;bit<32;bit++){
   auto b=boot,j=job;(field<64?b[field]:j[field-64])^=uint32_t(1)<<bit;tests.Check(1,b,j,0,phase);
  }
 }
 const unsigned edges[]={0,1,2,3,63,64,68,96,119,120,8191,8192,248319,248320,
  0x7ffffffd,0x7ffffffe,0x7fffffff,0x80000000,0xfffffff8,UINT32_MAX};
 for(unsigned op=1;op<=12;op++)for(unsigned previous:edges)for(unsigned phase:edges){
  auto [b,j]=Operation(1,op);j[1]=previous+1;tests.Check(1,b,j,previous,phase);
 }
 tests.Check(0,baseBoot,baseJob,0,0,0);tests.Check(1,baseBoot,baseJob,0,0,0);
 tests.Check(1,baseBoot,baseJob,0,0,GrammarMachine::Boot,0);
 tests.Check(1,baseBoot,baseJob,0,0,GrammarMachine::Boot,GrammarMachine::Boot);
 printf("PASS actual frozen old+new emitted Boot/Job grammar/%llu cases/%llu accepted/%llu refused/ALL119 worker arguments/"
  "ALL128x32 mutations across8 operations+unsigned/null/alias/ABI/stack-only writes;synthetic DATA arguments ONLY,"
  "NO acquired owner,connected model,native,hardware,timing or Work grant\n",
  (unsigned long long)tests.cases,(unsigned long long)tests.yes,(unsigned long long)tests.no);
}
