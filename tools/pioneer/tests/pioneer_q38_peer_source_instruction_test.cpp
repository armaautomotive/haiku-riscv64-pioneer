/* SPDX-License-Identifier: MIT */
// Isolated ACTUAL emitted audit functions, synthetic private arguments and
// generic read-packet endpoint. NOT acquired native ownership/model/card IO.
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
namespace P=PioneerQ38MixedRecurrentBlockProjectionLayout;
namespace C=PioneerQ38MixedRecurrentCohortLayout;
struct AuditImage {
 std::vector<uint8_t> text,terminal;std::array<unsigned,3> entry={};
 AuditImage(const char* directory,const char* mapping):text(Read((std::string(directory)+"/text.bin").c_str())),
  terminal(Read((std::string(directory)+"/terminal.bin").c_str())){
  std::ifstream in(mapping);for(auto& address:entry){in>>std::hex>>address;assert(in.good() && address>=0x40000 && address<0x40000+text.size());}
 }
};
static unsigned Source(unsigned i){return !i?0x100:i<=6?0x118+(i-1)*4:i<=12?0x130+(i-7)*4:i==13?0x150:i==14?0x154:0x16c;}
static unsigned Counter(unsigned i){const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return offsets[i];}
struct AuditMachine:Machine {
 enum {Owner=0x6e800,Tracked=0x6e400,Return=0x60004};
 const AuditImage& image;std::map<unsigned,uint32_t> memory,peer;
 std::array<uint32_t,10> command={};std::array<uint32_t,8> counters={};
 std::vector<std::pair<unsigned,unsigned>> packets;
 std::vector<std::pair<unsigned,unsigned>> sourceReads;
 std::vector<std::pair<unsigned,unsigned>> externalReads;
 unsigned kind,worker,writes=0,minimumStack=0x70000;
 AuditMachine(const AuditImage& rom,unsigned selected,unsigned w):image(rom),kind(selected),worker(w){
  for(unsigned i=0;i<64;i++){
   memory[R::Boot+i*4]=memory[Owner+i*4]=0x31400000U+i*0x129U;
   memory[R::Header+i*4]=memory[Owner+768+i*4]=0x21400000U+i*0x135U;
  }
  memory[R::Header]=memory[Owner+768]=PioneerQ38MixedProjectionRootLayout::Magic;
  memory[R::Header+9*4]=memory[Owner+768+9*4]=129;
  for(unsigned i=0;i<128;i++)memory[R::Outer+i*4]=memory[Owner+256+i*4]=0x12340000U+i*0x117U;
  memory[R::Outer+48*4]=memory[Owner+256+48*4]=7;memory[R::Doorbell]=7;memory[Owner+1024]=1;
  for(unsigned i=0;i<16;i++){
   memory[0xffb20000U+Source(i)]=memory.at(Owner+(16+i)*4);
   memory[R::Header+(16+i)*4]=memory[Owner+768+(16+i)*4]=memory.at(Owner+(16+i)*4);
   unsigned value=0x76540000U+i*0x113U+w*0x100;
   peer[0xffb20000U+Source(i)]=value;memory[P::PeerBoots+w*256+(16+i)*4]=value;
  }
  for(unsigned i=0;i<119*8;i++)memory[P::Parts+i*4]=memory[P::PartsShadow+i*4]=0x123000U+i;
  for(unsigned i=0;i<119;i++)memory[P::Rows+i*4]=memory[P::RowsShadow+i*4]=0x321000U+i;
  for(unsigned i=0;i<16;i++)memory[P::MatrixHeader+i*4]=memory[P::HeaderShadow+i*4]=0x456000U+i;
  for(unsigned i=0;i<8;i++)memory[P::Banks+i*4]=memory[P::BankShadow+i*4]=0x654000U+i;
  for(unsigned i=0;i<4;i++)memory[0xffb20040U+i*0x800]=0;
  for(unsigned i=0;i<32;i++)memory[0xffb20240U+i*4]=0;
  for(unsigned i=0;i<8;i++)memory[Tracked+i*4]=counters[i]=0xfffffff8U;
  for(unsigned i=0;i<64;i++)memory[P::Summary+i*4]=memory[C::Summary+i*4]=0;
  regs[1]=Return;regs[2]=0x70000;regs[27]=Owner;pc=image.entry[kind];
  if(kind==2){regs[10]=Tracked;regs[11]=w;}
  else{regs[10]=Owner+768;regs[11]=Tracked;regs[12]=w;}
 }
 unsigned StepLimit()const override{return 2000000;}
 bool CodeAddress(unsigned a)const override{return (a>=0x40000 && a-0x40000<image.text.size()) || (a>=0x60000 && a<0x60008);}
 unsigned Index(unsigned a)override{
  assert(CodeAddress(a) && !(a&3));l1[8180]=a<0x60000?Word(image.text,a-0x40000):Word(image.terminal,a-0x60000);return 8180;
 }
 uint32_t Load(uint32_t a)override{
  assert(!(a&3));if(CodeAddress(a))return l1[Index(a)];
  uint32_t value=0;bool found=false;
  if(a>=0xffb20000U && a<0xffb20028U){value=command[(a-0xffb20000U)/4];found=true;}
  for(unsigned i=0;i<8;i++)if(a==0xffb20000U+Counter(i)){value=counters[i];found=true;break;}
  if(!found){
   if(!memory.count(a))fprintf(stderr,"UNMAPPED isolated audit load%x PC%x kind%u\n",a,pc,kind);
   assert(memory.count(a));value=memory.at(a);
  }
  if(a<0x6e000 || a>=0x70000)externalReads.emplace_back(a,value);
  return value;
 }
 void Submit(){
  unsigned remote=command[0],local=command[3],bytes=command[8];
  assert(!command[1] && command[2]==PioneerQ38MixedWorkerLayout::Peer(worker+1) && !command[4]
   && command[5]==129 && !command[6] && !command[7] && !command[9]
   && local==P::Scratch+(remote&31) && bytes==4); // Blackhole MMIO is FOUR BYTES ONLY
  packets.emplace_back(remote,bytes);
  for(unsigned i=0;i<bytes/4;i++){
   assert(peer.count(remote+i*4));unsigned value=peer.at(remote+i*4);
   memory[local+i*4]=value;sourceReads.emplace_back(remote+i*4,value);
  }
  for(unsigned i=0;i<8;i++)counters[i]+=i==1?(bytes+63)/64:i==0 || i==2 || i==3 || i==6?1U:0U;
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3));
  if(a>=0xffb20000U && a<0xffb20028U){command[(a-0xffb20000U)/4]=value;return;}
  if(a==0xffb20040U){assert(value==1);Submit();return;}
  bool stack=a>=0x6f000 && a<0x70000;
  bool tracked=a>=Tracked && a<Tracked+32;
  bool summary=(a>=P::Summary && a<P::Summary+256) || (a>=C::Summary && a<C::Summary+256);
  assert(stack || tracked || summary);if(stack && a<minimumStack)minimumStack=a;
  memory[a]=value;writes++;
 }
 bool Run(){
  while(pc!=Return && pc!=0x60000)Step();bool okay=pc==Return;
  if(okay){assert(regs[2]==0x70000 && regs[27]==Owner && regs[1]==Return);for(unsigned i=0;i<8;i++)assert(memory.at(Tracked+i*4)==counters[i]);}
  unsigned packetsBefore=packets.size(),writesBefore=writes;
  for(unsigned i=0;i<2048;i++)Step();
  assert(pc==(okay?Return:0x60000) && packets.size()==packetsBefore && writes==writesBefore);
  return okay;
 }
};
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);AuditImage baseline(argv[1],argv[3]),candidate(argv[2],argv[4]);unsigned faults=0;
 for(unsigned kind=0;kind<3;kind++){
  for(unsigned worker:{0U,95U,118U}){
   auto a=std::make_unique<AuditMachine>(baseline,kind,worker),b=std::make_unique<AuditMachine>(candidate,kind,worker);
   assert(a->Run() && b->Run() && a->packets.size()==16 && a->packets==b->packets && a->sourceReads==b->sourceReads
    && a->externalReads==b->externalReads && (kind==1?b->steps<a->steps:b->steps==a->steps));
   printf("PASS emitted source audit%u worker%u ALL external reads/values/order EXACT,16x4B packets unchanged,instructions%u->%u,stack%u/%u;NOT model/native/physical/TPS\n",
    kind,worker,a->steps-2048,b->steps-2048,0x70000-a->minimumStack,0x70000-b->minimumStack);
  }
  for(unsigned word=0;word<16;word++){
   auto a=std::make_unique<AuditMachine>(baseline,kind,0),b=std::make_unique<AuditMachine>(candidate,kind,0);
   unsigned address=0xffb20000U+Source(word),mask=1U<<(word*7%32);a->peer.at(address)^=mask;b->peer.at(address)^=mask;
   assert(!a->Run() && !b->Run() && a->packets.size()==word+1 && a->packets==b->packets && a->externalReads==b->externalReads);faults++;
  }
  for(unsigned address:{unsigned(R::Boot),unsigned(R::Header),unsigned(R::Outer),unsigned(R::Doorbell),0xffb20100U,0xffb20040U,0xffb20240U,unsigned(AuditMachine::Tracked)}){
   auto a=std::make_unique<AuditMachine>(baseline,kind,0),b=std::make_unique<AuditMachine>(candidate,kind,0);
   a->memory.at(address)^=1;b->memory.at(address)^=1;assert(!a->Run() && !b->Run() && a->packets.empty() && b->packets.empty() && a->externalReads==b->externalReads);faults++;
  }
 }
 printf("PASS isolated emitted original/candidate source audits:9 positive functions/workers,%u source/root metadata/quiet/counter faults,actual immutable2048 inert steps;NO native acquisition/full-model/physical/Work/TPS grant\n",faults);
}
