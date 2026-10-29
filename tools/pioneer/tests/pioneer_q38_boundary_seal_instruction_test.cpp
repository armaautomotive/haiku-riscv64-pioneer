/* SPDX-License-Identifier: MIT */
// Isolated emitted boundary-profile functions with synthetic ABI arguments.
// NOT a genuine native owner,worker launch,model result or hardware timing.
#define PIONEER_Q38_PEER_SOURCE_INSTRUCTION_MACHINE_ONLY
#include "pioneer_q38_peer_source_instruction_test.cpp"
struct BoundaryMachine:AuditMachine {
 unsigned peerWrites=0,afterWriteAddress=0;
 BoundaryMachine(const AuditImage& image,unsigned kind,unsigned worker):AuditMachine(image,kind,worker){
  // Authenticated-looking DATA only. No native ownership is created here.
  for(unsigned i=0;i<8;i++)memory[R::Header+(32+i)*4]=memory[Owner+768+(32+i)*4]=memory.at(Owner+(8+i)*4);
 }
 void Store(uint32_t address,uint32_t value)override{
  if(address==0xffb20040U && command[7]==0x212){
   assert(value==1 && command[2]==129 && command[5]==PioneerQ38MixedWorkerLayout::Peer(worker+1)
    && !command[1] && !command[4] && !command[6] && !command[9] && command[8]==4
    && command[3]==PioneerQ38MixedWorkerLayout::Doorbell && command[0]==P::Scratch+(command[3]&31));
   assert(memory.at(command[0])==1);packets.emplace_back(command[3],4);peerWrites++;
   for(unsigned i=0;i<8;i++)counters[i]+=i==2 || i==4 || i==5 || i==6 || i==7?1U:0U;
   if(afterWriteAddress)memory.at(afterWriteAddress)^=1;
   return;
  }
  AuditMachine::Store(address,value);
 }
 void ReleaseEntry(unsigned entry){
  pc=entry;regs[10]=kind==2?Tracked:Owner+768;regs[11]=kind==2?worker:Tracked;
  if(kind==2){regs[12]=PioneerQ38MixedWorkerLayout::Doorbell;regs[13]=4;regs[14]=1;}
  else{regs[12]=worker;regs[13]=PioneerQ38MixedWorkerLayout::Doorbell;regs[14]=4;regs[15]=1;}
  memory[P::Scratch+(PioneerQ38MixedWorkerLayout::Doorbell&31)]=1;
 } // isolated raw function arguments BEFORE its first instruction only
};
static std::array<unsigned,3> Releases(const char* file){
 std::ifstream in(file);std::array<unsigned,3> result={};unsigned unused=0;
 for(unsigned i=0;i<3;i++){in>>std::hex>>unused;assert(in.good());}
 for(auto& address:result){in>>std::hex>>address;assert(in.good() && address>=0x40000 && address<0x60000);}
 return result;
}
static void OwnedDenseFrame(const AuditImage& image,const char* file){
 std::ifstream in(file);unsigned entry=0,seal=0,attention=0,unused=0;
 for(unsigned i=0;i<6;i++)in>>std::hex>>unused;
 in>>std::hex>>entry>>seal>>attention;assert(in.good() && entry>=0x40000 && seal>=0x40000 && attention>=0x40000);
 auto runWithoutTraffic=[](BoundaryMachine& m,bool okay){
  const auto before=m.memory;const auto command=m.command;const auto counters=m.counters;
  assert(m.Run()==okay && m.packets.empty() && !m.peerWrites && m.command==command && m.counters==counters);
  // An actual fault call saves registers on the private stack. Forbid all
  // NON-stack changes,not those legitimate compiler-generated stack stores.
  for(const auto& item:before)if(item.first<0x6f000 || item.first>=0x70000)assert(m.memory.at(item.first)==item.second);
  for(const auto& item:m.memory)if(item.first<0x6f000 || item.first>=0x70000)assert(before.at(item.first)==item.second);
 };
 auto good=std::make_unique<BoundaryMachine>(image,1,0);good->memory[AuditMachine::Owner+1024]=5;good->pc=entry;
 runWithoutTraffic(*good,true);assert(good->regs[10]==AuditMachine::Owner+768);
 for(unsigned phase:{0U,1U,4U,6U,13U}){
  auto bad=std::make_unique<BoundaryMachine>(image,1,0);bad->memory[AuditMachine::Owner+1024]=phase;bad->pc=entry;
  runWithoutTraffic(*bad,false);
 }
 for(unsigned owner:{0U,0x6dffcU,0x6e001U,0x6fc00U,UINT32_MAX}){
  auto bad=std::make_unique<BoundaryMachine>(image,1,0);bad->regs[27]=owner;bad->pc=entry;
  runWithoutTraffic(*bad,false);
 }
 // A byte-identical caller copy is STILL refused. The fix supplies the
 // owning descriptor;it does not broaden the packet guard's authority.
 auto copy=std::make_unique<BoundaryMachine>(image,1,0);copy->memory[AuditMachine::Owner+1024]=5;
 for(unsigned i=0;i<64;i++)copy->memory[0x6fc00+i*4]=copy->memory.at(AuditMachine::Owner+768+i*4);
 copy->pc=seal;copy->regs[10]=0x6fc00;runWithoutTraffic(*copy,false);
 // Observe the actual attention entry calling the emitted owner getter.
 // Its synthetic invalid DATA shape must subsequently fault before traffic.
 auto call=std::make_unique<BoundaryMachine>(image,1,0);call->memory[AuditMachine::Owner+1024]=5;call->pc=attention;
 bool observed=false;while(call->pc!=0x60000 && call->pc!=AuditMachine::Return){observed|=call->pc==entry;call->Step();}
 assert(observed && call->pc==0x60000 && call->packets.empty());
 puts("PASS boundary owned-dense descriptor/emitted phase5 getter+10 wrong-phase/range/alignment refusals/byte-identical caller copy STILL refused/actual attention entry calls getter then synthetic invalid DATA shape refuses BEFORE traffic;UNIT ABI ONLY,NOT positive attention/native/model/physical/TPS qualification");
}
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);AuditImage baseline(argv[1],argv[3]),candidate(argv[2],argv[4]);
 const auto oldRelease=Releases(argv[3]),newRelease=Releases(argv[4]);unsigned immediate=0,boundaries=0,postRelease=0,mmio=0;
 OwnedDenseFrame(candidate,argv[4]);
 for(unsigned kind=0;kind<3;kind++){
  for(unsigned worker:{0U,95U,118U}){
   auto a=std::make_unique<BoundaryMachine>(baseline,kind,worker),b=std::make_unique<BoundaryMachine>(candidate,kind,worker);
   assert(a->Run() && b->Run() && a->packets==b->packets && a->sourceReads==b->sourceReads && b->steps<a->steps);
   printf("PASS boundary-profile source audit%u worker%u SAME16x4B packets+values,instructions%u->%u,stack%u/%u;metadata rescan timing intentionally DIFFERENT,NOT model/native/physical/TPS\n",
    kind,worker,a->steps-2048,b->steps-2048,0x70000-a->minimumStack,0x70000-b->minimumStack);
   auto x=std::make_unique<BoundaryMachine>(baseline,kind,worker),y=std::make_unique<BoundaryMachine>(candidate,kind,worker);
   x->ReleaseEntry(oldRelease[kind]);y->ReleaseEntry(newRelease[kind]);
   assert(x->Run() && y->Run() && x->peerWrites==1 && y->peerWrites==1 && x->packets==y->packets);boundaries++;
  }
  for(unsigned word=0;word<16;word++){
   auto a=std::make_unique<BoundaryMachine>(baseline,kind,0),b=std::make_unique<BoundaryMachine>(candidate,kind,0);
   unsigned address=0xffb20000U+Source(word);a->peer.at(address)^=1;b->peer.at(address)^=1;
   assert(!a->Run() && !b->Run() && a->packets==b->packets && a->packets.size()==word+1);immediate++;
  }
  for(unsigned address:{0xffb20100U,0xffb20040U,0xffb20240U,unsigned(AuditMachine::Tracked),unsigned(R::Doorbell)}){
   auto a=std::make_unique<BoundaryMachine>(baseline,kind,0),b=std::make_unique<BoundaryMachine>(candidate,kind,0);
   a->memory.at(address)^=1;b->memory.at(address)^=1;
   assert(!a->Run() && !b->Run() && a->packets.empty() && b->packets.empty());immediate++;
  }
  // These are deliberately deferred in the new packet profile,NOT ignored
  // validation failures:the complete check must refuse before ANY release.
  for(unsigned address:{unsigned(R::Boot),unsigned(R::Header+4),unsigned(R::Outer)}){
   auto ordinary=std::make_unique<BoundaryMachine>(candidate,kind,0);ordinary->memory.at(address)^=1;
   assert(ordinary->Run() && ordinary->packets.size()==16 && !ordinary->peerWrites);
   auto before=std::make_unique<BoundaryMachine>(candidate,kind,0);before->memory.at(address)^=1;
   before->ReleaseEntry(newRelease[kind]);assert(!before->Run() && before->packets.empty() && !before->peerWrites);boundaries++;
  }
  // Mutate only after the actual isolated doorbell packet. The emitted
  // post-release proof must fault without adopting counters or retrying.
  for(unsigned address:{unsigned(R::Boot),unsigned(R::Header+4),unsigned(R::Outer),0xffb20100U,0xffb20240U,unsigned(R::Doorbell)}){
   auto after=std::make_unique<BoundaryMachine>(candidate,kind,0);after->afterWriteAddress=address;
   after->ReleaseEntry(newRelease[kind]);assert(!after->Run() && after->peerWrites==1 && after->packets.size()==1);
   for(unsigned i=0;i<8;i++)assert(after->memory.at(AuditMachine::Tracked+i*4)==0xfffffff8U);
   postRelease++;
  }
  // Neither a wider MMIO read nor an MMIO write can escape the packet
  // guard. These are isolated raw ABI calls,not a worker/native grant.
  for(bool write:{false,true}){
   auto invalid=std::make_unique<BoundaryMachine>(candidate,kind,0);invalid->ReleaseEntry(newRelease[kind]);
   invalid->regs[kind==2?12:13]=0xffb20100U;invalid->regs[kind==2?13:14]=write?4:8;
   invalid->regs[kind==2?14:15]=write?1:0;
   assert(!invalid->Run() && invalid->packets.empty() && !invalid->peerWrites);mmio++;
  }
 }
 printf("PASS boundary-profile emitted9 source comparisons/%u immediate source/counter/quiet/nonce refusals/%u positive+metadata-refused release boundaries/%u post-release fault-and-retain cases/%u MMIO-width-or-write refusals;actual2048 inert instructions each;NO native/whole-model/physical/Work grant\n",immediate,boundaries,postRelease,mmio);
}
