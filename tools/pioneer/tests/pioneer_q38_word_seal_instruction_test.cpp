/* SPDX-License-Identifier: MIT */
// ISOLATED emitted-function equivalence/profiling ONLY. Synthetic private
// arguments are unit-test inputs, NEVER a genuinely acquired ROOT owner,
// completed layer, initialized model, native launch or inference result.
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerQ38MixedRecurrentBlockRootLayout.h"
#include "../PioneerQ38MixedLiveBlockRootLayout.h"
#include <array>
#include <map>
#include <memory>
#include <string>
#include <utility>
namespace R=PioneerQ38MixedRecurrentBlockRootLayout;
namespace D=PioneerQ38MixedLiveBlockRootLayout;
struct SealImage {
 std::vector<uint8_t> text,terminal;std::array<unsigned,4> entries={};
 SealImage(const char* directory,const char* map):text(Read((std::string(directory)+"/text.bin").c_str())),
  terminal(Read((std::string(directory)+"/terminal.bin").c_str())){
  std::ifstream input(map);for(auto& entry:entries){input>>std::hex>>entry;assert(input.good() && entry>=0x40000 && entry<0x40000+text.size());}
 }
};
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
struct SealMachine:Machine {
 const SealImage& image;std::map<unsigned,unsigned> memory;
 std::vector<std::pair<unsigned,unsigned>> reads;
 enum {Owner=0x6e800,Return=0x60004};
 unsigned kind,writes=0;
 SealMachine(const SealImage& rom,unsigned selected):image(rom),kind(selected){
  static_assert(sizeof(R::PrivateSeal)==1028 && sizeof(D::PrivateSeal)==1028,"own unit argument grammar");
  static_assert(unsigned(R::Boot)==unsigned(D::Boot) && unsigned(R::Outer)==unsigned(D::Outer)
   && unsigned(R::Header)==unsigned(D::Header) && unsigned(R::Doorbell)==unsigned(D::Doorbell),"shared DATA addresses only");
  for(unsigned i=0;i<64;i++){
   memory[R::Boot+i*4]=memory[Owner+i*4]=0x31400000U+i*0x129U;
   memory[R::Header+i*4]=memory[Owner+768+i*4]=0x21400000U+i*0x135U;
  }
  for(unsigned i=0;i<128;i++)memory[R::Outer+i*4]=memory[Owner+256+i*4]=0x12340000U+i*0x117U;
  memory[R::Outer+48*4]=memory[Owner+256+48*4]=7;memory[R::Doorbell]=7;memory[Owner+1024]=1;
  for(unsigned i=0;i<16;i++){
   unsigned value=memory.at(Owner+(16+i)*4);memory[0xffb20000+Source(i)]=value;
   memory[R::Header+(16+i)*4]=memory[Owner+768+(16+i)*4]=value;
  }
  for(unsigned i=0;i<4;i++)memory[0xffb20040+i*0x800]=0;
  for(unsigned i=0;i<32;i++)memory[0xffb20240+i*4]=0;
  regs[1]=Return;regs[2]=0x70000;regs[27]=Owner;regs[10]=Owner+768;pc=image.entries[kind];
 }
 bool CodeAddress(unsigned a)const override{return (a>=0x40000 && a-0x40000<image.text.size()) || (a>=0x60000 && a<0x60008);}
 unsigned Index(unsigned a)override{
  assert(CodeAddress(a) && !(a&3));l1[8180]=a<0x60000?Word(image.text,a-0x40000):Word(image.terminal,a-0x60000);return 8180;
 }
 unsigned StepLimit()const override{return 20000;}
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(CodeAddress(a))return l1[Index(a)];
  if(!memory.count(a))fprintf(stderr,"UNMAPPED isolated seal LOAD%x PC%x kind%u\n",a,pc,kind);
  assert(memory.count(a));unsigned value=memory.at(a);
  // Exclude private synthetic arguments and compiler spill slots. Compare
  // EVERY external metadata/MMIO read including value and exact order.
  if(a<0x6e000 || a>=0x70000)reads.emplace_back(a,value);
  return value;
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3) && a>=0x6f000 && a<0x70000); // stack ONLY, no NoC/metadata/output/status writes
  memory[a]=value;writes++;
 }
 bool Run(){
  while(pc!=Return && pc!=0x60000)Step();
  bool pass=pc==Return;
  if(pass){
   assert(regs[2]==0x70000 && regs[27]==Owner && regs[1]==Return);
   for(unsigned saved:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U})assert(!regs[saved]);
  }
  unsigned before=unsigned(reads.size()),written=writes;
  for(unsigned i=0;i<2048;i++)Step();
  assert(pc==(pass?Return:0x60000) && reads.size()==before && writes==written);
  return pass;
 }
};
#ifndef PIONEER_Q38_WORD_SEAL_MACHINE_ONLY
struct Result {unsigned steps=0,reads=0;};
static Result Check(const SealImage& baseline,const SealImage& candidate,unsigned kind,
 unsigned address=0,unsigned mask=0,int expected=1,unsigned pointer=SealMachine::Owner){
 auto a=std::make_unique<SealMachine>(baseline,kind),b=std::make_unique<SealMachine>(candidate,kind);
 if(address){assert(a->memory.count(address) && b->memory.count(address));a->memory[address]^=mask;b->memory[address]^=mask;}
 a->regs[27]=b->regs[27]=pointer;
 bool x=a->Run(),y=b->Run();assert(x==y && x==bool(expected) && a->reads==b->reads);
 // Only returned calls provide the comparable whole check-routine counts.
 if(!address && pointer==SealMachine::Owner){
  assert(x && b->steps<a->steps);
  printf("PASS isolated emitted seal%u external read trace%zu EXACT; instructions baseline%u unrolled%u (excludes2048 inert steps);NO model/native/physical/TPS proof\n",
   kind,a->reads.size(),a->steps-2048,b->steps-2048);
 }
 return {b->steps-2048,unsigned(b->reads.size())};
}
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);SealImage baseline(argv[1],argv[3]),candidate(argv[2],argv[4]);unsigned failures=0;
 for(unsigned kind=0;kind<4;kind++){
  Check(baseline,candidate,kind);
  for(unsigned base:{unsigned(R::Boot),unsigned(R::Header),unsigned(R::Outer)}){
   unsigned count=base==R::Outer?128:64;
   for(unsigned i=0;i<count;i++)for(unsigned bit=0;bit<32;bit++){
    Check(baseline,candidate,kind,base+i*4,1U<<bit,0);failures++;
   }
  }
  for(unsigned i=0;i<16;i++)for(unsigned bit=0;bit<32;bit++){
   Check(baseline,candidate,kind,0xffb20000+Source(i),1U<<bit,0);failures++;
  }
  for(unsigned i=0;i<256;i++){
   Check(baseline,candidate,kind,SealMachine::Owner+i*4,1U,0);failures++;
  }
  for(unsigned bit=0;bit<32;bit++){Check(baseline,candidate,kind,R::Doorbell,1U<<bit,0);failures++;}
  for(unsigned phase:{0U,16U,0xffffffffU}){Check(baseline,candidate,kind,SealMachine::Owner+1024,1U^phase,0);failures++;}
  for(unsigned pointer:{0U,0x6dffcU,0x6fc00U,0xffffffffU}){Check(baseline,candidate,kind,0,0,0,pointer);failures++;}
  if(kind&1){
   for(unsigned i=0;i<4;i++)for(unsigned mask:{1U,128U,256U,0x80000000U}){
    Check(baseline,candidate,kind,0xffb20040+i*0x800,mask,0);failures++;
   }
   for(unsigned i=0;i<32;i++)for(unsigned mask:{1U,128U,256U,0x80000000U}){
    bool fault=bool(mask&255);Check(baseline,candidate,kind,0xffb20240+i*4,mask,!fault);failures+=fault;
   }
  }
 }
 printf("PASS isolated emitted check functions: %u single-bit/phase/pointer/quiet refusals;fail-fast external traces EXACT,stack writes ONLY,inert2048;NOT genuine owner/acquisition/full graph/Work/performance qualification\n",failures);
}
#endif
