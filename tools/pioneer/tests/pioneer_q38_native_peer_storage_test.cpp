/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_NATIVE_PEER_TEST_BRIDGE 1
#include "pioneer_q38_native_peer_test_bridge.h"
#define PIONEER_TENSIX_MACHINE_ONLY
#include "pioneer_tensix_image_test.cpp"
#include "../PioneerQ38TokenPrograms.h"
#include "../PioneerQ38GlobalWorkerLayout.h"
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeDramSamplesProbe.h"
#include <array>
#include <memory>
struct NativePeerBytes {
 std::array<std::vector<uint32_t>,2> l1;
 std::array<std::array<uint32_t,16>,2> source={};
 std::array<std::array<uint32_t,10>,2> command={};
 std::array<std::array<uint32_t,8>,2> counters={};unsigned reads=0,puts=0,events=0;
 NativePeerBytes(){for(unsigned w=0;w<2;w++){
  l1[w].resize(0x180000/4);for(unsigned i=0;i<l1[w].size();i++)l1[w][i]=0xa53179U*(i+1)^0x51470000U*(w+1);
  for(unsigned i=0;i<16;i++)source[w][i]=i?0x12340000+i+w*0x100:0x16005;
  command[w][5]=PioneerQ38GlobalWorkerLayout::Peer(w+1);counters[w].fill(0xfffffff8);
 }}
 Q38NativePeerTestBytes Endpoints(){
  Q38NativePeerTestBytes b;b.context=this;
  b.word=[](void* c,unsigned worker,uint32_t address)->uint32_t&{
   auto& m=*static_cast<NativePeerBytes*>(c);assert(worker && worker<=2 && !(address&3) && address<0x180000);return m.l1[worker-1].at(address/4);
  };
  b.noc=[](void* c,unsigned worker,unsigned offset){
   auto& m=*static_cast<NativePeerBytes*>(c);assert(worker && worker<=2);unsigned w=worker-1;
   if(offset<40 && !(offset&3))return m.command[w][offset/4];
   for(unsigned i=0;i<16;i++)if(offset==BlackholeDramSourceOffset(i))return m.source[w][i];
   const unsigned counters[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
   for(unsigned i=0;i<8;i++)if(offset==counters[i])return m.counters[w][i];
   if(offset==0x40 || offset==0x840 || offset==0x1040 || offset==0x1840 || (offset>=0x240 && offset<0x2c0))return 0U;
   assert(false);return UINT32_MAX;
  };
  b.events=[](void* c,unsigned worker,const uint32_t* delta,const uint32_t* command,unsigned mask,bool pendingChanged,bool pending){
   auto& m=*static_cast<NativePeerBytes*>(c);assert(worker && worker<=2 && !pendingChanged && !pending && !(mask&~1023U));m.events++;
   for(unsigned i=0;i<8;i++)assert(!delta[i]);
   for(unsigned i=0;i<10;i++)if(mask&(1U<<i))m.command[worker-1][i]=command[i];
  };
  b.read=[](void* c,unsigned,uint32_t,uint8_t*,unsigned){static_cast<NativePeerBytes*>(c)->reads++;return false;};
  b.put=[](void* c,unsigned,uint32_t,uint32_t){static_cast<NativePeerBytes*>(c)->puts++;return false;};
  b.clock=[](void*){return UINT64_C(1)<<40;};return b;
 }
 void Install(unsigned worker){
  namespace P=PioneerQ38TokenPrograms;namespace W=PioneerQ38GlobalWorkerLayout;
  auto& raw=l1[worker-1];for(unsigned i=0;i<P::CodeWords(worker);i++){
   uint32_t address=P::CodeAddress(worker,i),word;assert(P::ProgramWord(worker,address,word));raw.at(address/4)=word;
  }
  uint32_t boot[64]={};boot[0]=W::Magic;boot[1]=1;boot[2]=64;boot[3]=worker;boot[4]=W::Peer(worker);boot[5]=source[worker-1][0];boot[6]=8192;boot[7]=64;
  for(unsigned i=0;i<4;i++)boot[8+i*2]=i+1;
  for(unsigned i=0;i<16;i++)boot[16+i]=source[worker-1][i];for(unsigned i=40;i<46;i++)boot[i]=W::BootWord(i);
  assert(W::BootShape(boot));for(unsigned i=0;i<64;i++)raw.at((W::Boot+i*4)/4)=boot[i];
  raw.at(W::Doorbell/4)=0;for(unsigned i=0;i<16;i++)raw.at((W::Status+i*4)/4)=0;
 } // pure component-test code/boot DATA,NOT native held staging or a native owner
};
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&Read;(void)&Word;
 Machine unchanged;assert(sizeof(unchanged.l1)==32768 && !unchanged.steps); // other TU must not replace this class/vtable/layout
 NativePeerBytes memory;const auto original=memory.l1;auto endpoints=memory.Endpoints();
 Q38NativePeerTestCpu first(argv[1],argv[2],1,endpoints),second(argv[1],argv[2],2,endpoints);
 assert(memory.l1==original && !first.Stats().steps && !second.Stats().steps && first.Stats().pc==0x6000 && second.Stats().pc==0x6000);
 first.Link(second);second.Link(first);memory.Install(1);memory.Install(2);
 for(auto* cpu:{&first,&second})for(unsigned round=0;round<4096;round++){
  const auto before=cpu->Stats();if(before.pc==0x2004c || before.pc==0x20050)break;cpu->Advance(256);
 }
 for(unsigned w=1;w<=2;w++){
  const auto s=(w==1?first:second).Stats();assert((s.pc==0x2004c || s.pc==0x20050) && s.steps && s.statusWrites==17
   && !s.matrixInstructions && !s.moveInstructions && !s.floatOps && !s.requests && !s.responses && !s.pending && !s.sequence && !s.active && s.minStack>=0x6e000);
  for(unsigned i=0;i<16;i++)assert(memory.l1[w-1].at((PioneerQ38GlobalWorkerLayout::Status+i*4)/4)==(i==3?unsigned(PioneerQ38GlobalWorkerLayout::Magic):0U));
 }
 const unsigned events=memory.events;memory.counters[0][0]++;memory.command[0][1]^=1;
 first.Advance(256);assert(memory.counters[0][0]==0xfffffff9 && memory.command[0][1]==1 && memory.events==events);
 assert(!memory.reads && !memory.puts && !unchanged.steps && sizeof(unchanged.l1)==32768);
 puts("PASS TWO virgin full matrix/GDN/Conv/attention ISA engines on SAME external L1 from construction/ALL generic constructor mailbox+code+operand writes DISCARDED/no readiness or private history copied/actual QGW1 boots+17status stores+WAIT/physical counter+command faults PERSIST across real idle instructions/other TU old Machine layout intact;COMPONENT TEST ONLY,NOT native binding/119 cohort/matrix/STOP/END/physical/Work/TPS");
}
