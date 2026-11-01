/* SPDX-License-Identifier: MIT */
// Actual emitted guards,synthetic ABI arguments ONLY. Faults execute the
// real report writer and terminal loop;never supply a completion or return.
#define PIONEER_Q38_WORD_SEAL_MACHINE_ONLY
#include "pioneer_q38_word_seal_instruction_test.cpp"
struct IdentityMachine:SealMachine {
 static_assert(R::Status==0x75440,"actual fault writer status layout");
 unsigned faultLoop=0,faultWrites=0;
 IdentityMachine(const SealImage& image,unsigned kind,const char* mapping):SealMachine(image,kind){
  std::ifstream input(mapping);unsigned entry;
  for(unsigned i=0;i<4;i++)input>>std::hex>>entry;
  input>>std::hex>>faultLoop;assert(input.good() && faultLoop>=0x4001c && faultLoop<0x60000);
  memory[Owner+1024]=2;
  for(unsigned i=0;i<8;i++)memory[R::Header+(32+i)*4]=memory[Owner+768+(32+i)*4]=memory.at(Owner+(8+i)*4);
 }
 void Store(uint32_t address,uint32_t value)override{
  if(address==0x75460 || address==0x75448){
   assert((address==0x75460 && pc==faultLoop-24 && !faultWrites)
    || (address==0x75448 && pc==faultLoop-8 && faultWrites==1 && value==0x51465431));
   memory[address]=value;faultWrites++;return;
  }
  SealMachine::Store(address,value); // compiler stack ONLY
 }
 bool Run(){
  while(pc!=Return && pc!=faultLoop)Step();
  const bool okay=pc==Return;
  if(okay){
   assert(!faultWrites && regs[2]==0x70000 && regs[27]==Owner && regs[1]==Return);
   for(unsigned saved:{8U,9U,18U,19U,20U,21U,22U,23U,24U,25U,26U})assert(!regs[saved]);
  }else assert(faultWrites==2 && memory.at(0x75448)==0x51465431);
  const auto frozen=memory;const auto visible=reads;const auto stackWrites=writes;
  const auto terminal=pc;
  for(unsigned i=0;i<2048;i++)Step();
  assert(pc==terminal && memory==frozen && reads==visible && writes==stackWrites && faultWrites==(okay?0U:2U));
  return okay;
 }
 void StopOuter(){
  for(unsigned i=0;i<128;i++)memory[R::Outer+i*4]=memory[Owner+256+i*4]=0;
  const unsigned words[][2]={{0,R::Magic},{1,1},{2,128},{5,129},{48,7},{49,119},{72,R::Stop}};
  for(const auto& pair:words)memory[R::Outer+pair[0]*4]=memory[Owner+256+pair[0]*4]=pair[1];
  for(unsigned i=16;i<40;i++)memory[R::Outer+i*4]=memory[Owner+256+i*4]=memory.at(Owner+768+i*4);
 }
};
struct IdentityImages {
 SealImage old,candidate;const char *oldMap,*newMap;unsigned cases=0,refusals=0,deferred=0;
 IdentityImages(char** argv):old(argv[1],argv[3]),candidate(argv[2],argv[4]),oldMap(argv[3]),newMap(argv[4]){}
 using Mutation=std::vector<std::pair<unsigned,unsigned>>;
 std::pair<unsigned,unsigned> Check(unsigned kind,const Mutation& changes={},bool oldOkay=true,bool newOkay=true,
  unsigned phase=2,unsigned owner=SealMachine::Owner,unsigned frame=SealMachine::Owner+768,bool stopped=false,bool sameReads=true){
  auto a=std::make_unique<IdentityMachine>(old,kind,oldMap),b=std::make_unique<IdentityMachine>(candidate,kind,newMap);
  for(auto* m:{a.get(),b.get()}){
   if(stopped)m->StopOuter();
   m->memory[SealMachine::Owner+1024]=phase;
   for(const auto& change:changes){assert(m->memory.count(change.first));m->memory.at(change.first)^=change.second;}
   m->regs[27]=owner;m->regs[10]=frame;
  }
  const bool x=a->Run(),y=b->Run();
  if(x!=oldOkay || y!=newOkay){
   fprintf(stderr,"IDENTITY_GUARD kind%u phase%u expected%u/%u actual%u/%u firstFault%x/%x\n",
    kind,phase,unsigned(oldOkay),unsigned(newOkay),unsigned(x),unsigned(y),a->pc,b->pc);assert(false);
  }
  // All physical source+doorbell reads remain immediate,in exact order.
  // A changed early identity failure legitimately reaches fewer reads.
  if(oldOkay==newOkay && sameReads)assert(a->reads==b->reads);
  if(!sameReads){
   assert(kind==1 && !oldOkay && !newOkay && changes.size()==1 && a->reads.size()==1 && a->reads[0].first==R::Doorbell);
   const unsigned source=(changes[0].first-owner)/4-16;
   assert(source<16 && b->reads.size()==source+2 && b->reads[0]==a->reads[0]);
   for(unsigned i=0;i<=source;i++)assert(b->reads[i+1].first==0xffb20000U+Source(i)
    && b->reads[i+1].second==b->memory.at(0xffb20000U+Source(i)));
  }
  cases++;refusals+=!newOkay;deferred+=!oldOkay && newOkay;
  return {a->steps-2048,b->steps-2048};
 }
};
#ifndef PIONEER_Q38_PACKET_IDENTITY_MACHINE_ONLY
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);IdentityImages tests(argv);
 using S=SealMachine;const unsigned owner=S::Owner;
 for(unsigned kind=0;kind<2;kind++){
  for(unsigned phase=2;phase<15;phase++){
   const auto counts=tests.Check(kind,{},true,true,phase);
   if(kind==1){assert(counts.second<counts.first);if(phase==2)printf("PASS actual packet guard host instructions%u->%u;ALL live source/doorbell reads EXACT;NOT device cycles/TPS\n",counts.first,counts.second);}
  }
  tests.Check(kind,{},true,true,15,owner,owner+768,true);
  for(unsigned phase:{0U,16U,UINT32_MAX})tests.Check(kind,{},false,false,phase);
  for(unsigned pointer:{0U,0x6dffcU,0x6fc00U,UINT32_MAX})tests.Check(kind,{},false,false,2,pointer);
  for(unsigned bit=0;bit<32;bit++)tests.Check(kind,{{R::Doorbell,1U<<bit}},false,false);
  for(unsigned i=0;i<16;i++)for(unsigned bit=0;bit<32;bit++)
   tests.Check(kind,{{0xffb20000U+Source(i),1U<<bit}},false,false);
 }
 tests.Check(1,{},false,false,2,owner,owner+772);
 for(unsigned nonce:{0U,0x7fffffffU,UINT32_MAX})tests.Check(1,{{owner+256+48*4,7U^nonce}},false,false);
 // The cold full audit has no frame yet;new packet paths must refuse it.
 IdentityImages::Mutation cold;
 for(unsigned i=0;i<64;i++){
  IdentityMachine m(tests.old,0,tests.oldMap);
  cold.emplace_back(owner+768+i*4,m.memory.at(owner+768+i*4));
  cold.emplace_back(R::Header+i*4,m.memory.at(R::Header+i*4));
 }
 tests.Check(0,cold,true,true,1);tests.Check(1,cold,false,false,1);
 // Every mirrored private frame identity mutation is deferred by packet,
 // but rejected by the actual full guard even if Header is also altered.
 for(unsigned i=0;i<24;i++)for(unsigned bit=0;bit<32;bit++){
  const unsigned word=i<16?16+i:32+i-16;
  const IdentityImages::Mutation one={{owner+768+word*4,1U<<bit}};
  tests.Check(1,one,false,true);
  tests.Check(0,one,false,false);
  auto mirrored=one;mirrored.emplace_back(R::Header+word*4,1U<<bit);
  tests.Check(0,mirrored,true,false);
  // Private boot is not a trusted replacement value. Source damage must
  // still fail on the next actual physical read;nonce damage is deferred
  // only until the full immutable/binding boundary,never accepted there.
  const unsigned boot=i<16?16+i:8+i-16;
  const IdentityImages::Mutation privateBoot={{owner+boot*4,1U<<bit}};
  tests.Check(1,privateBoot,false,i>=16,2,owner,owner+768,false,i>=16);
  tests.Check(0,privateBoot,false,false);
  if(i>=16){
   auto mirroredBoot=privateBoot;mirroredBoot.emplace_back(R::Boot+boot*4,1U<<bit);
   tests.Check(0,mirroredBoot,true,false);
  }
 }
 // ALL original external immutable mirrors still fail at the full audit.
 for(unsigned base:{unsigned(R::Boot),unsigned(R::Header),unsigned(R::Outer)}){
  const unsigned words=base==R::Outer?128:64;
  for(unsigned i=0;i<words;i++)for(unsigned bit=0;bit<32;bit++)tests.Check(0,{{base+i*4,1U<<bit}},false,false);
 }
 printf("PASS emitted packet/full identity boundary:%u cases/%u immediate-or-boundary refusals/%u explicit deferred packet identities;actual fault writer+2048 inert,stack-only normal writes;synthetic ABI ONLY,NOT acquired owner/original model/native/physical/Work/TPS\n",tests.cases,tests.refusals,tests.deferred);
}
#endif
