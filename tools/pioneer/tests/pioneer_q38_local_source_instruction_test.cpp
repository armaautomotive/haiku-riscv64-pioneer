/* SPDX-License-Identifier: MIT */
// Isolated emitted functions and synthetic ABI data ONLY. No ROOT/native
// owner,worker/completion receipt or model product can be manufactured here.
#define PIONEER_Q38_PEER_SOURCE_INSTRUCTION_MACHINE_ONLY
#include "pioneer_q38_peer_source_instruction_test.cpp"
#include "../PioneerQ38GlobalWorkerLayout.h"
namespace G=PioneerQ38GlobalWorkerLayout;
struct LocalSourceMachine:AuditMachine {
 unsigned peerWrites=0,afterWriteAddress=0;
 LocalSourceMachine(const AuditImage& image,unsigned kind,unsigned worker):AuditMachine(image,kind,worker){
  for(unsigned i=0;i<8;i++)memory[R::Header+(32+i)*4]=memory[Owner+768+(32+i)*4]=memory.at(Owner+(8+i)*4);
  for(unsigned i=0;i<64;i++){
   const unsigned address=P::PeerBoots+worker*256+i*4;
   unsigned value=0;
   if(i<8){const unsigned h[]={G::Magic,1,64,worker+1,G::Peer(worker+1),peer.at(0xffb20100),8192,64};value=h[i];}
   else if(i<16)value=memory.at(Owner+(8+i-8)*4);
   else if(i<32)value=peer.at(0xffb20000U+Source(i-16));
   else value=G::BootWord(i);
   memory[address]=value;
   peer[G::Boot+i*4]=memory.at(address);
  }
  std::array<uint32_t,64> boot;for(unsigned i=0;i<64;i++)boot[i]=peer.at(G::Boot+i*4);
  assert(G::BootShape(boot.data()));
  for(unsigned i=0;i<64;i++)peer[G::Job+i*4]=0;
  for(unsigned i=0;i<16;i++)peer[G::Status+i*4]=i==3?unsigned(G::Magic):0;
  peer[G::Doorbell]=0;peer[0xffb121b0]=0x46800;peer[0xffb13140]=0x2004c;
  for(unsigned i=0;i<4;i++)peer[0xffb20040+i*0x800]=0;
  for(unsigned i=0;i<32;i++)peer[0xffb20240+i*4]=0;
  for(unsigned i=0;i<8;i++)peer[0xffb20000U+Counter(i)]=0xfffffff0U;
  memory[P::Nonces+worker*4]=0;
  memory[P::Parts+worker*32+5*4]=memory[P::PartsShadow+worker*32+5*4]=peer.at(0xffb20100);
  for(auto pair:{std::pair<unsigned,unsigned>{7,119},{40,0},{42,0},{43,kind?5U:10U},{48,1}})
   memory[R::Header+pair.first*4]=memory[Owner+768+pair.first*4]=pair.second;
 }
 void Store(uint32_t address,uint32_t value)override{
  if(address==0xffb20040U){
   if(command[7]==0x212){
    assert(value==1 && command[2]==129 && command[5]==G::Peer(worker+1) && command[3]==G::Doorbell
     && command[8]==4 && !command[1] && !command[4] && !command[6] && !command[9]
     && command[0]==P::Scratch+(G::Doorbell&31) && memory.at(command[0])==1);
    packets.emplace_back(G::Doorbell,4);peerWrites++;
    for(unsigned i=0;i<8;i++)counters[i]+=i==2 || i==4 || i==5 || i==6 || i==7?1U:0U;
    if(afterWriteAddress)memory.at(afterWriteAddress)^=1;
    return;
   }
   assert(value==1 && !command[1] && command[2]==G::Peer(worker+1)
    && command[3]==P::Scratch+(command[0]&31) && !command[4] && command[5]==129
    && !command[6] && !command[7] && !command[9]); // observations ONLY,no release/write endpoint
   const unsigned remote=command[0],bytes=command[8];assert(bytes==4 || bytes==64 || bytes==256);
   if(remote>=0xff000000U)assert(bytes==4);
   packets.emplace_back(remote,bytes);
   for(unsigned i=0;i<bytes/4;i++)memory[command[3]+i*4]=peer.at(remote+i*4);
   for(unsigned i=0;i<16;i++)if(remote==0xffb20000U+Source(i))sourceReads.emplace_back(remote,peer.at(remote));
   for(unsigned i=0;i<8;i++)counters[i]+=i==1?(bytes+63)/64:i==0 || i==2 || i==3 || i==6?1U:0U;
   return;
  }
  if((address>=P::PeerBoots && address<P::PeerBoots+119*256)
   || (address>=P::PeerJobs && address<P::PeerJobs+119*256)
   || (address>=P::Nonces && address<P::Nonces+119*4)
   || (address>=P::Counters && address<P::Counters+119*32)){memory[address]=value;return;}
  AuditMachine::Store(address,value);
 }
 void Call(unsigned entry){pc=entry;regs[1]=Return;regs[10]=Owner+768;regs[11]=Tracked;regs[12]=worker;}
 void Release(unsigned entry){Call(entry);regs[13]=G::Doorbell;regs[14]=4;regs[15]=1;memory[P::Scratch+(G::Doorbell&31)]=1;}
};
static std::array<unsigned,6> Entries(const char* file){
 std::ifstream in(file);std::array<unsigned,6> result={};
 for(auto& a:result){in>>std::hex>>a;assert(in.good() && a>=0x40000 && a<0x60000);}return result;
}
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);
 AuditImage baseline(argv[1],argv[3]),candidate(argv[2],argv[4]);
 const auto oldEntry=Entries(argv[3]),newEntry=Entries(argv[4]);unsigned bootFaults=0,sourceFaults=0,nextFaults=0,earlyFaults=0,releaseFaults=0,afterRelease=0,mmioFaults=0;
 for(unsigned kind=0;kind<2;kind++){
  for(unsigned worker:{0U,95U,118U}){
   auto a=std::make_unique<LocalSourceMachine>(baseline,kind,worker),b=std::make_unique<LocalSourceMachine>(candidate,kind,worker);
   assert(a->Run() && b->Run() && a->packets.size()==17 && b->packets.size()==1 && a->sourceReads.size()==16 && b->sourceReads.empty()
    && a->packets.front()==b->packets.front() && b->steps<a->steps);
   printf("PASS emitted ROOT post-completion boot seal%u worker%u/64 boot words still checked/packets17->1 ONLY in this helper/instructions%u->%u/stack%u/%u;NOT model/native/physical/TPS\n",
    kind,worker,a->steps-2048,b->steps-2048,0x70000-a->minimumStack,0x70000-b->minimumStack);
   auto x=std::make_unique<LocalSourceMachine>(baseline,kind,worker),y=std::make_unique<LocalSourceMachine>(candidate,kind,worker);
   x->Call(oldEntry[2+kind]);y->Call(newEntry[2+kind]);
   assert(x->Run() && y->Run() && x->packets.size()==y->packets.size()+16 && x->sourceReads.size()==32 && y->sourceReads.size()==16);
   printf("PASS emitted ROOT actual virgin Admit%u worker%u/ALL16 physical pre-launch source reads retained/ONLY duplicate post-audit removed/packets%zu->%zu;synthetic endpoints,NOT a native/real worker launch grant\n",kind,worker,x->packets.size(),y->packets.size());
  }
  for(unsigned address:{unsigned(R::Doorbell),0xffb20100U,0xffb20040U,0xffb20240U,unsigned(AuditMachine::Tracked)}){
   auto a=std::make_unique<LocalSourceMachine>(baseline,kind,0),b=std::make_unique<LocalSourceMachine>(candidate,kind,0);
   a->memory.at(address)^=1;b->memory.at(address)^=1;
   assert(!a->Run() && !b->Run() && a->packets.empty() && b->packets.empty());earlyFaults++;
  }
  // Existing boundary policy defers immutable metadata checks during
  // read-only observations,but MUST reject BEFORE an actual release packet.
  for(unsigned address:{unsigned(R::Boot),unsigned(R::Header+4),unsigned(R::Outer)}){
   for(unsigned variant=0;variant<2;variant++){
    const auto& image=variant?candidate:baseline;const auto& entry=variant?newEntry:oldEntry;
    auto read=std::make_unique<LocalSourceMachine>(image,kind,0);read->memory.at(address)^=1;
    assert(read->Run() && read->packets.size()==(variant?1U:17U) && !read->peerWrites);
    auto release=std::make_unique<LocalSourceMachine>(image,kind,0);release->memory.at(address)^=1;
    release->Release(entry[4+kind]);assert(!release->Run() && release->packets.empty() && !release->peerWrites);releaseFaults++;
   }
  }
  for(unsigned variant=0;variant<2;variant++){
   const auto& image=variant?candidate:baseline;const auto& entry=variant?newEntry:oldEntry;
   auto release=std::make_unique<LocalSourceMachine>(image,kind,0);release->Release(entry[4+kind]);
   assert(release->Run() && release->peerWrites==1 && release->packets.size()==1);
   for(unsigned address:{unsigned(R::Boot),unsigned(R::Header+4),unsigned(R::Outer),0xffb20100U,0xffb20240U,unsigned(R::Doorbell)}){
    auto bad=std::make_unique<LocalSourceMachine>(image,kind,0);bad->afterWriteAddress=address;bad->Release(entry[4+kind]);
    assert(!bad->Run() && bad->peerWrites==1 && bad->packets.size()==1);
    for(unsigned i=0;i<8;i++)assert(bad->memory.at(AuditMachine::Tracked+i*4)==0xfffffff8U);afterRelease++;
   }
   for(bool write:{false,true}){
    auto bad=std::make_unique<LocalSourceMachine>(image,kind,0);bad->Release(entry[4+kind]);
    bad->regs[13]=0xffb20100U;bad->regs[14]=write?4:8;bad->regs[15]=write?1:0;
    assert(!bad->Run() && bad->packets.empty() && !bad->peerWrites);mmioFaults++;
   }
  }
  for(unsigned word=0;word<64;word++){
   auto a=std::make_unique<LocalSourceMachine>(baseline,kind,0),b=std::make_unique<LocalSourceMachine>(candidate,kind,0);
   a->peer.at(G::Boot+word*4)^=1;b->peer.at(G::Boot+word*4)^=1;
   assert(!a->Run() && !b->Run() && a->packets==b->packets && a->packets.size()==1 && a->sourceReads.empty() && b->sourceReads.empty());bootFaults++;
  }
  for(unsigned word=0;word<16;word++){
   const unsigned address=0xffb20000U+Source(word);
   auto a=std::make_unique<LocalSourceMachine>(baseline,kind,0),b=std::make_unique<LocalSourceMachine>(candidate,kind,0);
   a->Call(oldEntry[2+kind]);b->Call(newEntry[2+kind]);a->peer.at(address)^=1;b->peer.at(address)^=1;
   assert(!a->Run() && !b->Run() && a->packets==b->packets && a->packets.size()==word+5 && a->sourceReads==b->sourceReads && b->sourceReads.size()==word+1);sourceFaults++;
   // Changed timing is explicit:the next source-audit entry MUST reject.
   // These are the real emitted BootSeal then Admit ABI entries in ONE
   // synthetic endpoint instance,not permission to launch a native worker.
   auto next=std::make_unique<LocalSourceMachine>(candidate,kind,0);next->peer.at(address)^=1;
   assert(next->Run() && next->packets.size()==1 && next->sourceReads.empty());
   next->Call(newEntry[2+kind]);assert(!next->Run() && next->packets.size()==word+6 && next->sourceReads.size()==word+1);nextFaults++;
  }
 }
 printf("PASS isolated emitted post-completion policy/12 positive boot+Admit functions/4 positive release packets/%u immediate source-queue-counter faults/%u metadata refusals BEFORE release/%u post-release fault-retentions/%u MMIO-width-write refusals/%u boot mismatches/%u real Admit physical-audit failures/%u explicit next-Admit refusals/actual2048 inert instructions each;worker-local faults and genuine whole ROOT/model/END qualification are SEPARATE,NO physical/Work/TPS grant\n",earlyFaults,releaseFaults,afterRelease,mmioFaults,bootFaults,sourceFaults,nextFaults);
}
