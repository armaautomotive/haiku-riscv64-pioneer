/* SPDX-License-Identifier: MIT */
// Actual emitted guards on synthetic ABI inputs. No acquired owner,model,
// processor history,native/device access,completion or timing is supplied.
#define PIONEER_Q38_PACKET_IDENTITY_MACHINE_ONLY
#include "pioneer_q38_packet_identity_boundary_instruction_test.cpp"
struct ReadSourceMachine:IdentityMachine {
 ReadSourceMachine(const SealImage& image,unsigned selected,const char* mapping)
  :IdentityMachine(image,selected,mapping){
  // Real FIFO/TID guards require all FIFOs empty and the captured enable
  // bit. Coherent fixture mirrors are TEST arguments,not initialization.
  memory[0xffb20064U]=0x10101010U;memory[0xffb20068U]=0;
  const uint32_t enabled=memory.at(Owner+16*4)|0x10000U;
  for(unsigned at:{unsigned(R::Boot)+16*4,unsigned(Owner)+16*4,unsigned(R::Header)+16*4,unsigned(Owner)+768+16*4,
                  0xffb20100U})memory.at(at)=enabled;
  regs[11]=selected==1; // Actual new signature: write=false only for reads.
 }
};
struct ReadSourceTests {
 SealImage image;const char* mapping;unsigned cases=0,refusals=0,deferred=0;
 ReadSourceTests(char** argv):image(argv[1],argv[2]),mapping(argv[2]){}
 using Mutation=std::vector<std::pair<unsigned,unsigned>>;
 unsigned Check(unsigned kind,bool okay,const Mutation& changes={},unsigned phase=2,
                unsigned owner=SealMachine::Owner,unsigned frame=SealMachine::Owner+768){
  auto m=std::make_unique<ReadSourceMachine>(image,kind,mapping);
  m->memory[SealMachine::Owner+1024]=phase;m->regs[27]=owner;m->regs[10]=frame;
  for(const auto& change:changes){assert(m->memory.count(change.first));m->memory.at(change.first)^=change.second;}
  const bool actual=m->Run();
  if(actual!=okay){fprintf(stderr,"READ_SOURCE guard%u phase%u expected%u actual%u pc%x\n",kind,phase,unsigned(okay),unsigned(actual),m->pc);assert(false);}
  if(kind==2 && okay){
   assert(m->reads.size()==1 && m->reads[0].first==R::Doorbell);
  }
  if(kind==1 && okay){
   assert(m->reads.size()==17 && m->reads[0].first==R::Doorbell);
   for(unsigned i=0;i<16;i++)assert(m->reads[i+1].first==0xffb20000U+Source(i));
  }
  cases++;refusals+=!okay;return m->steps-2048;
 }
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);ReadSourceTests t(argv);
 using S=SealMachine;const unsigned owner=S::Owner;
 for(unsigned phase=2;phase<=14;phase++)for(unsigned kind=0;kind<3;kind++)t.Check(kind,true,{},phase);
 const unsigned write=t.Check(1,true),read=t.Check(2,true);assert(read<write);
 printf("PASS emitted read guard HOST instructions%u versus write%u;1 doorbell versus17 ordered external reads;NOT device cycles/TPS\n",read,write);
 // Every live source bit can defer through read metadata,never through
 // the actual write guard or actual full immutable+compute boundary.
 for(unsigned i=0;i<16;i++)for(unsigned bit=0;bit<32;bit++){
  const ReadSourceTests::Mutation fault={{0xffb20000U+Source(i),1U<<bit}};
  t.Check(2,true,fault);t.Check(1,false,fault);t.Check(0,false,fault);t.deferred++;
 }
 for(unsigned kind=0;kind<3;kind++){
  for(unsigned phase:{0U,16U,UINT32_MAX})t.Check(kind,false,{},phase);
  for(unsigned pointer:{0U,0x6dffcU,0x6fc00U,UINT32_MAX})t.Check(kind,false,{},2,pointer);
  for(unsigned bit=0;bit<32;bit++)t.Check(kind,false,{{R::Doorbell,1U<<bit}});
 }
 for(unsigned kind:{1U,2U}){
  t.Check(kind,false,{},1);
  t.Check(kind,false,{},2,owner,owner+772);
  for(unsigned nonce:{0U,0x7fffffffU,UINT32_MAX})t.Check(kind,false,{{owner+256+48*4,7U^nonce}});
  // Even coherently disabling all compared FIFO-mode mirrors never lets
  // the read path borrow a serial-only configuration.
  const ReadSourceTests::Mutation disabled={{unsigned(R::Boot)+16*4,0x10000U},{owner+16*4,0x10000U},
    {unsigned(R::Header)+16*4,0x10000U},{owner+768+16*4,0x10000U},{0xffb20100U,0x10000U}};
  t.Check(kind,false,disabled);
 }
 for(unsigned base:{unsigned(R::Boot),unsigned(R::Header),unsigned(R::Outer)}){
  const unsigned words=base==R::Outer?128:64;
  for(unsigned i=0;i<words;i++)for(unsigned bit=0;bit<32;bit++)t.Check(0,false,{{base+i*4,1U<<bit}});
 }
 printf("PASS emitted read-source boundary/%u cases/%u immediate-or-full refusals/%u explicit read-only deferrals;"
        "actual fault writer+2048 inert/ABI preserved/normal stack writes ONLY;"
        "synthetic ABI,NOT connected release,original model,native,device,Work or TPS proof\n",t.cases,t.refusals,t.deferred);
}
