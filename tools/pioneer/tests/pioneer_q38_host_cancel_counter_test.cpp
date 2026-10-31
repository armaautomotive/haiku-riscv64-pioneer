/* SPDX-License-Identifier: MIT */
// HOST transport regression ONLY. Execute the pinned, unmodified persistent
// ROOT from RESET with the SAME119 workers and synthetic raw model inputs.
// The candidate endpoint delivers ONE RAW32 cancel write plus its documented
// destination-NIU counter events. No PC, result, receipt or owner is supplied.
#if !defined(PIONEER_Q38_GLOBAL_CHAIN_PUSHED_COMPLETION) || !defined(PIONEER_Q38_TEST_CANCELLABLE_ROOT)
#error Explicit HOST cancellable QGW3 profile required
#endif
#define PIONEER_Q38_TOKEN_FLOW_MACHINE_ONLY
#include "pioneer_q38_token_flow_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerComputeFleet.h"
#ifdef PIONEER_Q38_TEST_POSTED_CANCEL_CONTROL
#define PIONEER_Q38_POSTED_CANCEL_TEST 1
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38PostedCancel.h"
#endif

struct HostCancelCounterMachine:TokenFlowMachine {
 using TokenFlowMachine::TokenFlowMachine;
 enum {HostCases=
#ifdef PIONEER_Q38_TEST_POSTED_CANCEL_CONTROL
  3
#else
  2
#endif
 };
 unsigned endFirst=0,endLast=0,cases=0,mode=0,packetsAtWrite=0,peerWritesAtWrite=0;
 bool forked=false,delivered=false,continued=false,faultROM=false;
 std::array<uint32_t,4> beforeWrite={};
#ifdef PIONEER_Q38_TEST_POSTED_CANCEL_CONTROL
 struct PostedEndpoint {
  HostCancelCounterMachine& m;uint32_t tlb[2][3]={};
  unsigned writes=0,modes=0,reads=0;bool pending=false;uint64_t clock=100;
  std::array<unsigned,3> posted={};
  explicit PostedEndpoint(HostCancelCounterMachine& actual):m(actual){
   for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)tlb[s][w]=PioneerQ38PostedCancel::Expected(s,w,false);
  }
  bool PostedCancelScope(const PioneerQ38PostedCancel&){return m.mode==3 && m.forked && !m.continued
   && m.pc>=m.endFirst && m.pc<m.endLast && !m.allLayers && !m.committed && !m.stopJobs;}
  bool PostedCancelSourceGuard(){
   for(unsigned i=0;i<16;i++)if(m.source[i]!=m.boot[16+i])return false;
   return true;
  }
  bool PostedCancelTlbRead(unsigned s,unsigned w,uint32_t& out){out=tlb[s][w];return true;}
  bool PostedCancelControlWrite(bool post){tlb[0][2]=PioneerQ38PostedCancel::Expected(0,2,post);modes++;return true;}
  bool PostedCancelWordWrite(){assert(!writes && !pending);writes++;pending=true;return true;}
  bool PostedCancelWordRead(uint32_t& out){
   if(pending && ++reads==2){
    // A read may overtake a posted write. Deliver only on the second actual
    // endpoint read,using the actual selected TLB request class.
    const unsigned ordering=(tlb[0][2]>>6)&3;assert(ordering==2);
    m.memory[TL::CancelLocal]=TL::CancelMagic;for(auto& count:posted)count++;pending=false;
   }
   out=m.memory.at(TL::CancelLocal);return true;
  }
  uint64_t PostedCancelNow(){return clock;}void PostedCancelPause(){clock+=1000;}
 };
#endif
 void Deliver(unsigned network){
  assert(network<HostCases && !delivered && forked && !position && !layer && matrices==1
   && pushedMessages==6 && !allLayers && !committed && !stopJobs
   && !memory.at(TL::CancelLocal) && pc>=endFirst && pc<endLast);
  beforeWrite=pushedSlave;packetsAtWrite=packets;peerWritesAtWrite=peerWrites;
#ifdef PIONEER_Q38_TEST_POSTED_CANCEL_CONTROL
  if(network==2){
   PostedEndpoint io(*this);PioneerQ38PostedCancel request;
   assert(request.Request(io) && request.Delivered() && !request.Active() && io.writes==1 && io.modes==2 && io.reads==2
    && pushedSlave==beforeWrite && io.posted[0]==1 && io.posted[1]==1 && io.posted[2]==1);
   for(unsigned s=0;s<2;s++)for(unsigned w=0;w<3;w++)assert(io.tlb[s][w]==PioneerQ38PostedCancel::Expected(s,w,false));
   delivered=true;return;
  }
#endif
  memory[TL::CancelLocal]=TL::CancelMagic;delivered=true;
  // One completed, nonposted, one-flit NoC write at the destination NIU:
  // STARTED, DATA_WORD_RECEIVED, REQ_RECEIVED, ACK_SENT each advance once.
  // NoC1 has a distinct NIU; it does not alter these NoC0 counters.
  if(!network)for(auto& counter:pushedSlave)counter++;
 }
 uint32_t Load(uint32_t address)override{
  if(!forked && address==0xffb20000+SlaveOffset(0) && pushedMessages==6
   && pc>=endFirst && pc<endLast){
   forked=true;
   const unsigned branches=HostCases;
   for(unsigned network=0;network<branches;network++){
    BeforeFork();fflush(nullptr);const pid_t childPid=fork();assert(childPid>=0);
    if(!childPid){mode=network+1;Deliver(network);return TokenFlowMachine::Load(address);}
    int status=0;assert(waitpid(childPid,&status,0)==childPid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
   assert(cases==branches && !delivered && !mode && !memory.at(TL::CancelLocal));
  }
  return TokenFlowMachine::Load(address);
 }
 void Store(uint32_t address,uint32_t value)override{
  const bool submit=address==0xffb20040;
  if(submit && mode==1)assert(false && "NoC0 cancel must fault before another ROOT packet");
  const bool next=submit && forked && command[7]==0x212 && command[5]==G::Peer(1)
   && command[3]==M::Doorbell && matrices==2;
  TokenFlowMachine::Store(address,value);
  if(next){assert(mode!=1 && !continued && (!mode?cases>=2:delivered));continued=true;}
 }
 void Check(){
  assert(forked && !allLayers && !committed && !stopJobs && !position && !layer
   && minSp>=0x6e000 && minStack>=0x6e000);
  if(mode==1){
   assert(delivered && !continued && pc==0x60000 && packets==packetsAtWrite && peerWrites==peerWritesAtWrite);
   for(unsigned i=0;i<4;i++)assert(pushedSlave[i]==beforeWrite[i]+1);
   const auto counters=counts;const auto incoming=pushedSlave;const auto commands=command;
   for(unsigned i=0;i<2048;i++)StepToken();
   assert(pc==(faultROM?0x4002cU:0x60000U) && packets==packetsAtWrite && peerWrites==peerWritesAtWrite
    && counts==counters && pushedSlave==incoming && command==commands);
   if(faultROM){
    assert(memory.at(RB::Status+8)==0x51465431 && memory.at(RB::Status+32)==0x4e050);
    puts("PASS exact fault ROM/actual error51465431 saved_LINK0004e050 MATCH observed physical DATA/common receipt-closure fault site;LINK is NOT a retired PC or unique failing-branch proof");
   }
   puts("PASS same pinned ROOT/actual first3 QGW3 jobs+6 receipt writes/ONE modeled NoC0 cancel delivery/strict incoming frontier faults/2048 instructions limited to park or fault-report DATA/no following packet;NOT physical cause or model/native/Work/TPS qualification");
  }else{
   assert(continued && pc!=0x60000 && pc!=0x60004);
   if(mode==2){assert(delivered && pushedSlave==beforeWrite && memory.at(TL::CancelLocal)==TL::CancelMagic);
    puts("PASS modeled separate-NoC cancel control/SAME executed ROOT+119 histories/NoC0 frontier unchanged/next actual matrix doorbell;NOT an implemented NoC1 route or completed cancellation");
   }
#ifdef PIONEER_Q38_TEST_POSTED_CANCEL_CONTROL
   else if(mode==3){assert(delivered && pushedSlave==beforeWrite && memory.at(TL::CancelLocal)==TL::CancelMagic);
    puts("PASS actual posted-cancel controller at reached receipt closure/one modeled posted RAW32 write/delayed destination readback/normal TLB restored/unchanged strict NoC0 incoming frontier/SAME executed ROOT+119 histories/next actual matrix doorbell;NOT native/physical/Work/TPS qualification");
   }
#endif
   else{assert(!delivered && cases>=2 && !memory.at(TL::CancelLocal));
    puts("PASS unchanged no-host-write parent/next actual matrix doorbell;synthetic raw inputs,not a full layer/model/physical speed result");}
  }
 }
};

int main(int argc,char** argv){
 assert(argc==6 || (argc==7 && !strcmp(argv[6],"--fault-marker")));setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 auto m=std::make_unique<HostCancelCounterMachine>(argv[1],argv[2],argv[3]);
 m->endFirst=unsigned(strtoul(argv[4],nullptr,16));m->endLast=unsigned(strtoul(argv[5],nullptr,16));
 m->faultROM=argc==7;
 assert(m->endFirst>=0x40000 && m->endLast>m->endFirst && m->endLast<=0x6c000);
 // Decode the ACTUAL production TLB policy, not a separately supplied NoC.
 const auto policy=BlackholeDramFixedTlbWord(0,PioneerWorkerX(0),PioneerWorkerY(0),false,2);
 assert(((policy>>3)&1)==0 && ((policy>>6)&3)==1); // NoC0, Strict AXI
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->continued){
  m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 m->Check();
}
