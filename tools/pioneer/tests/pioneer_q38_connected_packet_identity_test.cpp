/* SPDX-License-Identifier: MIT */
// Actual original embedding+119 virgin histories. Fault COW at an actual
// executed packet guard;no supplied products,owner or processor history.
#include "pioneer_q38_token_original_fabric.h"
struct PacketIdentityFaultMachine:OriginalTokenFlowMachine {
 using OriginalTokenFlowMachine::OriginalTokenFlowMachine;
 unsigned entry=0,full=0,link=0,owner=0,fault=48,cases=0,doorbells=0,faultFrom=0;
 std::array<std::pair<unsigned,unsigned>,2> admission={};
 bool forked=false,injected=false,packetReturned=false,fullEntered=false,parentContinued=false;
 void Predecessor(){
  assert(pc==entry && !position && !layer && !completed && !allLayers && !committed && !stopJobs
   && embedded==1 && !matrices && !convJobs && !gdnJobs && Phase()==2);
  owner=regs[27];assert(owner>=0x6e000 && owner<=0x70000-sizeof(RB::PrivateSeal)
   && regs[10]==owner+768 && memory.at(owner+1024)==2);
  for(unsigned i=0;i<24;i++){
   const unsigned word=i<16?16+i:32+i-16,boot=i<16?16+i:8+i-16;
   assert(memory.at(owner+768+word*4)==memory.at(owner+boot*4)
    && memory.at(RB::Header+word*4)==memory.at(owner+768+word*4));
  }
  for(const auto& p:child)assert(!p->active && !p->sequence && p->GdnFinished(0) && !p->GdnFaulted()
   && !p->pending && !p->requests && !p->responses && !p->matrixInstructions);
 }
 void StepToken(){
  if(pc==entry && !forked && !position && !layer){
   Predecessor();forked=true;link=regs[1];
   assert(link>=0x40000 && link<0x60000 && !(link&3));
   puts("OBSERVED actual original embedding+119 virgin workers/Phase2 installed identity/BEFORE first packet guard;HOST COW only");
   for(unsigned index=0;index<48;index++){
    BeforeFork();fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
    if(!pid){
     injected=true;fault=index;const unsigned identity=index%24,word=identity<16?16+identity:32+identity-16;
     const unsigned mask=1U<<((index*7+5)%32);
     memory.at(owner+768+word*4)^=mask;
     if(index>=24)memory.at(RB::Header+word*4)^=mask;
     break; // Continue SAME actual not-yet-executed packet instruction.
    }
    int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));cases++;
   }
   if(!injected){assert(cases==48 && !doorbells);Predecessor();}
  }
  if(injected && pc==link)packetReturned=true;
  if(injected && pc==full){assert(packetReturned);fullEntered=true;}
  const unsigned prior=pc;OriginalTokenFlowMachine::StepToken();
  if(pc==0x60000)faultFrom=prior;
 }
 void Store(uint32_t address,uint32_t value)override{
  const bool packet=address==0xffb20040,write=command[7]==0x212;
  const unsigned remote=write?command[3]:command[0],route=write?command[5]:command[2];
  bool release=false;
  if(packet && write && remote==M::Doorbell)for(unsigned w=0;w<119;w++)if(route==G::Peer(w+1))release=true;
  if(release && forked)assert(!injected); // NO release after identity corruption.
  OriginalTokenFlowMachine::Store(address,value);
  if(release && forked){
   assert(!injected && !doorbells && child[0]->sequence==1);doorbells++;
   auto& p=*child[0];
   for(unsigned i=0;i<1000000 && !p.requests && !p.GdnFaulted();i++){cycles+=19;p.StepBlock();}
   assert(p.requests==1 && !p.GdnFaulted() && !p.matrixInstructions);parentContinued=true;
  }
 }
 void CheckFault(){
  printf("OBSERVED identity fault%u pc%x from%x packetReturn%u fullEntry%u link%x savedLink%x doorbells%u\n",
   fault,pc,faultFrom,unsigned(packetReturned),unsigned(fullEntered),link,regs[1],doorbells);
  assert(injected && fault<48 && forked && packetReturned && !doorbells && pc==0x60000
   && !allLayers && !committed && !stopJobs && original.Ready());
  if(fault%24<16)assert(fullEntered);
  else{
   // Cold Admit already compares the real worker boot nonce to the
   // descriptor BEFORE the next full seal. Require that actual earlier
   // refusal,not a fabricated full-audit observation or arbitrary fault.
   assert(!fullEntered && ((faultFrom>=admission[0].first && faultFrom<admission[0].first+admission[0].second)
    || (faultFrom>=admission[1].first && faultFrom<admission[1].first+admission[1].second)));
   const unsigned word=fault%24-16;
   assert(memory.at(RB::PeerBoots+(8+word)*4)==memory.at(owner+(8+word)*4)
    && memory.at(owner+768+(32+word)*4)!=memory.at(RB::PeerBoots+(8+word)*4));
  }
  for(const auto& p:child)assert(!p->active && !p->sequence && !p->requests && !p->responses && !p->matrixInstructions);
  const auto commands=command;const auto counters=counts;const auto writes=peerWrites,packetsAtFault=packets;
  for(unsigned i=0;i<40;i++)StepToken();
  assert(memory.at(0x75448)==0x51465431 && packets==packetsAtFault && command==commands && counts==counters);
  const auto frozen=memory;const auto terminal=pc;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==terminal && memory==frozen && packets==packetsAtFault && peerWrites==writes && command==commands && counts==counters);
  printf("PASS original packet identity%u/%s:actual packet RETURNS then %s faults/NO worker release,math,model commit/fault writer+2048 inert;NOT native/physical/Work/TPS\n",
   fault%24,fault>=24?"private+Header":"private",fullEntered?"next full audit":"earlier cold worker-boot nonce binding");
 }
};
int main(int argc,char** argv){
 assert(argc==11);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 puts("START SAME-open original27B authentication BEFORE packet-identity fault history;NO CPU products or hardware IO");
 WholeModelOriginalBytes original;assert(original.Open(argv[4]) && original.Ready());
 auto m=std::make_unique<PacketIdentityFaultMachine>(argv[1],argv[2],argv[3],original);
 m->entry=unsigned(strtoul(argv[5],nullptr,16));m->full=unsigned(strtoul(argv[6],nullptr,16));
 for(unsigned i=0;i<2;i++)m->admission[i]={unsigned(strtoul(argv[7+i*2],nullptr,16)),unsigned(strtoul(argv[8+i*2],nullptr,16))};
 for(const auto& range:m->admission)assert(range.first>=0x40000 && range.second && range.first+range.second<=0x60000
  && !((range.first|range.second)&3));
 assert(m->entry>=0x40000 && m->entry<0x60000 && m->full>=0x40000 && m->full<0x60000 && !((m->entry|m->full)&3));
 m->VerifyRawEndpoints();
 while(m->pc!=0x60000 && m->pc!=0x60004 && !m->parentContinued){
  m->cycles+=19;m->StepToken();if(!(m->steps&255))for(unsigned w=0;w<119;w++)m->Advance(w,64);
 }
 if(m->injected){m->CheckFault();_exit(0);}
 assert(m->forked && m->cases==48 && m->parentContinued && m->doorbells==1 && !m->allLayers && !m->committed
  && !m->stopJobs && original.Ready());
 puts("PASS original executed packet identity boundary/24 private+24 mirrored corruption children/unchanged parent first matrix request;NO transplanted history or supplied products;NOT full layer/native/physical/Work/TPS");
}
