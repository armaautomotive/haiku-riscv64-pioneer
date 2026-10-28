/* SPDX-License-Identifier: MIT */
// Actual cancellation variant from RESET, ONE ROOT+SAME119 histories.
// Model inputs are RAW only; the only host signal is the admitted cancel
// control word, never a private PC/register/nonce/product/completion hint.
#define PIONEER_Q38_TOKEN_FLOW_MACHINE_ONLY
#include "pioneer_q38_token_flow_test.cpp"
struct TokenCancelMachine:TokenFlowMachine {
 unsigned target,requested=0,fault=0;bool forked=false,injected=false;
 TokenCancelMachine(const char* image,const char* old,const char* worker,unsigned stage):TokenFlowMachine(image,old,worker),target(stage){
  assert(cancelBoundary && target>=1 && target<=3 && !memory.at(TL::CancelLocal));
 }
 void Store(uint32_t a,uint32_t v)override{
  TokenFlowMachine::Store(a,v);
  if(a!=0xffb20040)return;
  if(!requested && (target==1?embedded==1 && !releases:target==2?globalPartials==973:tokenPublications==1)){
   assert(!memory.at(TL::CancelLocal) && !position && !stopJobs);
   memory[TL::CancelLocal]=TL::CancelMagic;requested=1;expectCancelStop=true;
   printf("REQUEST RAW cancellation stage%u/actual model progress layers%u head%u records%u/NO private state supplied\n",target,allLayers,globalPartials,tokenPublications);
  }
  if(fault>=2 && !injected && command[7]==0 && command[2]==G::Peer(1) && child[0]->pc==child[0]->stopPC){
   unsigned remote=command[0],local=command[3];
   if(fault==2 && remote==G::Status){memory[local+8]=1;injected=true;}
   if(fault==3 && remote==0xffb121b0){memory[local]=0x46801;injected=true;}
   if(fault==4 && remote==0xffb20208){memory[local]^=1;injected=true;}
  }
 }
 bool RunCancel(){
  while(pc!=0x60000 && pc!=0x60004){
   cycles+=19;StepToken();
   if(!forked && stopJobs==1){
    assert(requested && ActualPrivate(29)==target && !position);forked=true;fflush(nullptr);
    for(unsigned bad=1;bad<=4;bad++){
     pid_t pid=fork();assert(pid>=0);
     if(!pid){fault=bad;if(bad==1){counts[7]++;injected=true;}break;}
     int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
    }
    if(!fault)puts("PASS ACTUAL 4 cancellation-terminal corruptions/from first real STOP release/SAME executed ROOT+119 history/unchanged positive parent;NOT native cleanup");
   }
   if(!(steps&255))for(unsigned w=0;w<119;w++)Advance(w,64);
  }
  return pc==0x60004;
 }
 void Inert(unsigned terminal){
  unsigned before=packets,writes=peerWrites;auto commands=command;auto physical=counts;
  for(unsigned i=0;i<2048;i++)StepToken();
  assert(pc==terminal && packets==before && peerWrites==writes && command==commands && counts==physical);
 }
 void VerifyFault(){
  assert(fault && injected && forked && pc==0x60000 && requested && stopJobs && stopJobs<=119
   && memory.at(TL::TerminalLocal)!=TL::TerminalMagic && committed==unsigned(target==3) && flips==committed
   && tokenPublications==committed && !position && ActualPrivate(18)==committed);
  Inert(0x60000);
  printf("PASS ACTUAL cancellation stage%u terminal fault%u/no terminal capability/NO invented STOP or cleanup/ROOT NoC+peer-write inert2048;in-flight workers UNKNOWN,not reusable\n",target,fault);fflush(nullptr);
 }
 void VerifyCancel(){
  unsigned done=target==1?1:64,heads=target==1?0:973,commits=target==3?1:0;
  assert(pc==0x60004 && requested && forked && !fault && !position && layer==done-1 && completed==done && allLayers==done
   && committed==commits && flips==commits && tokenPublications==commits && globalJobs==heads && globalPartials==heads
   && stopJobs==119 && ActualPrivate(18)==commits && ActualPrivate(19)==commits && ActualPrivate(29)==target && ActualPrivate(64)==7
   && minSp>=0x6e000 && minStack>=0x6e000);
  std::array<uint32_t,64> expected={};
  const unsigned head[]={TL::TerminalMagic,1,3,commits,0,119,done,heads,commits,1,target};
  for(unsigned i=0;i<11;i++)expected[i]=head[i];
  for(unsigned i=0;i<8;i++){expected[12+i]=boot[8+i];expected[36+i]=counts[i];}
  for(unsigned i=0;i<16;i++)expected[20+i]=source[i];
  for(unsigned i=0;i<64;i++)assert(memory.at(TL::TerminalLocal+i*4)==expected[i]);
  for(unsigned w=0;w<119;w++){
   const auto& p=*child[w];unsigned waves=target==1?0:w<21?9:8;
   assert(!p.active && p.pc==p.stopPC && p.sequence==nonceBefore[w]+waves+1 && p.memory.at(M::Status+16)==G::Stop
    && memory.at(RB::Nonces+w*4)==p.sequence);
  }
  if(target==1)assert(!kvWrites && !kvReadbacks && !sourceCopies && !kvFaults);
  Inert(0x60004);
  printf("PASS ACTUAL cancellation stage%u/real model layers%u+head%u/committed tokens%u/ALL119 actual STOP+source+job+counter closure/terminal read locally/inert2048/stack reserved%u written%u;SYNTHETIC raw model,NOT native/physical/Work/TPS or reusable mutable state\n",
   target,done,heads,commits,0x70000-minSp,0x70000-minStack);
 }
};
int main(int argc,char** argv){
 assert(argc==5);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 unsigned target=!strcmp(argv[4],"--layer")?1:!strcmp(argv[4],"--head")?2:!strcmp(argv[4],"--committed")?3:0;assert(target);
 auto m=std::make_unique<TokenCancelMachine>(argv[1],argv[2],argv[3],target);
 for(unsigned bad:{1U,unsigned(TL::CancelMagic),~0U}){
  fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){
   m->memory[TL::CancelLocal]=bad;assert(!m->RunCancel() && m->pc==0x60000 && !m->packets && !m->peerWrites && !m->releases);
   m->Inert(0x60000);_exit(0);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 puts("PASS ACTUAL cancellation initial3 nonzero RAW-control refusals/ZERO NoC+peer work/inert2048;not boundary cancellation proof");
 bool okay=m->RunCancel();if(m->fault){assert(!okay);m->VerifyFault();_exit(0);}
 assert(okay);m->VerifyCancel();
}
