/* SPDX-License-Identifier: MIT */
// Actual pinned parent instructions from reset through first returning call.
// All119 peers genuinely boot once. No completed owner/product is supplied.
#define PIONEER_Q38_GLOBAL_CHAIN_MACHINE_ONLY
#include "pioneer_q38_global_chain_instruction_test.cpp"
#include "../PioneerQ38TokenLayout.h"
namespace TL=PioneerQ38TokenLayout;
struct TokenBootstrapMachine:GlobalChainInstructionMachine {
 unsigned entry=0,embeddingReads=0,embeddingWrites=0,embeddingReadbacks=0,released=0,fault=0;
 bool injected=false;
 const unsigned firstToken=248319;
 std::unique_ptr<RPlan> expected=std::make_unique<RPlan>();
 std::array<uint32_t,5120> decoded={};
 TokenBootstrapMachine(const char* image,const char* old,const char* worker):GlobalChainInstructionMachine(image,old,worker){
  std::array<uint32_t,1920> sources={};for(unsigned i=0;i<16;i++)sources[i]=source[i];
  for(unsigned w=0;w<119;w++)for(unsigned i=0;i<16;i++)sources[(w+1)*16+i]=child[w]->boot[16+i];
  unsigned workspace=outer[9];
  assert(expected->Build(*canonical,0,0,firstToken,0,0,1001,1,{1,2,3,4},sources.data(),0,workspace+PioneerQ38GlobalFeedbackLayout::Offset));
  uint32_t session[128]={};const unsigned h[]={TL::Magic,1,128,0,workspace,2,2,firstToken};
  for(unsigned i=0;i<8;i++){session[i]=h[i];session[24+i]=boot[8+i];}
  for(unsigned i=0;i<16;i++)session[8+i]=source[i];
  session[32]=1;session[33]=248318;session[34]=248317;session[35]=8192;
  assert(TL::Session(session,boot.data(),0,workspace));
  for(unsigned i=0;i<128;i++)Put(outer[8],workspace+TL::SessionOffset+i*4,session[i]);
  for(unsigned i=0;i<16;i++)Put(outer[8],workspace+TL::PromptOffset+i*4,i?17:firstToken);
  std::ifstream symbols(std::string(image)+"/program.symbols");std::string line;
  while(std::getline(symbols,line)){
   std::istringstream row(line);std::string address,kind,name;row>>address>>kind>>name;
   if(name=="q38_return_recurrent_layer")entry=unsigned(std::stoul(address,nullptr,16));
  }
  assert(entry);
  PioneerQ38EmbeddingPlan plan;assert(plan.Build(*canonical,firstToken,0,0,{1,2,3,4},source.data(),17));const auto* d=plan.Descriptor();
  for(unsigned block=0;block<20;block++){
   uint8_t packed[144];float values[256];
   for(unsigned b=0;b<144;b++){
    unsigned address=d[11]+block*2304+b*16+d[13];packed[b]=uint8_t(Get(d[12],address&~3U)>>((address&3)*8));
   }
   assert(PioneerDecodeQ4K(packed,values));for(unsigned i=0;i<256;i++)decoded[block*256+i]=Bits(values[i]);
  }
 }
 void SubmitBootstrap(){
  packets++;const bool write=command[7]==0x212;unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0];
  unsigned route=write?command[5]:command[2],bytes=command[8];
  assert(bytes && bytes<=16384 && !(bytes&3) && !(remote&3) && local==RB::Scratch
   && !command[1] && !command[4] && !command[6] && !command[9] && (write?command[2]:command[5])==129 && (write || !command[7]));
  bool bank=false;for(unsigned b=0;b<7;b++)bank|=route==PioneerQ38ProjectionRoute(b);
  assert(bank); // Bootstrap/metadata have NO peer jobs, writes or releases.
  if(Phase()==0x300){
   namespace F=PioneerQ38GlobalFeedbackLayout;
   if(bytes==2304){assert(!write && embeddingReads<20);embeddingReads++;}
   if(remote>=outer[9]+F::Offset && remote<outer[9]+F::Offset+20480){
    unsigned at=remote-outer[9]-F::Offset;
    assert(route==outer[8] && at+bytes<=20480 && embeddingReads==20);
    for(unsigned i=0;i<5120;i++)assert(memory.at(F::Local+i*4)==decoded[i] && memory.at(F::Shadow+i*4)==decoded[i]);
    if(write || fault!=4)for(unsigned i=0;i<bytes/4;i++)assert((write?memory.at(local+i*4):Get(route,remote+i*4))==decoded[at/4+i]);
    if(write)embeddingWrites++;else {assert(embeddingWrites==2);embeddingReadbacks++;}
   }
  }
  for(unsigned i=0;i<bytes/4;i++)if(write)Put(route,remote+i*4,memory.at(local+i*4));else memory[local+i*4]=Get(route,remote+i*4);
  counts[write?4:0]++;counts[write?5:1]+=(bytes+63)>>6;counts[write?7:3]++;counts[2]++;counts[6]++;
  if(!injected && fault && ((fault<=3 && embeddingReads==1) || (fault>=4 && fault<=7 && embeddingWrites==1) || (fault==8 && embeddingReadbacks==1))){
   injected=true;
   if(fault==1 || fault==5)counts[3]++;
   if(fault==2)source[11]^=1;
   if(fault==3)memory[TL::SessionLocal+5*4]^=1;
   if(fault==4)Put(route,remote,Get(route,remote)^1); // accepted write corrupts GDDR, later readback MUST fail
   if(fault==6)memory[CL::Head+127*4]^=1;
   if(fault==7)memory[RB::Outer+13*4]^=1;
   if(fault==8)memory[local]^=1;
  }
 }
 void Store(uint32_t a,uint32_t value)override{
  assert(!(a&3) && !CodeAddress(a));
  if(Range(a,0xffb20000,40)){assert(!pending);command[(a-0xffb20000)/4]=value;return;}
  if(a==0xffb20040){assert(value==1 && !pending);SubmitBootstrap();return;}
  assert(memory.count(a));if(Range(a,0x6e000,8192))minStack=std::min(minStack,unsigned(a));
  if(a==RB::Doorbell){
   assert(value==1 && !released++ && embeddingReads==20 && embeddingWrites==2 && embeddingReadbacks==2);
   for(unsigned i=0;i<128;i++)assert(memory.at(RB::Outer+i*4)==expected->Outer()[i]);
  }
  memory[a]=value;
 }
 bool RunBootstrap(){
  while(pc!=entry && pc!=0x60000){assert(steps<200000000);StepScalar();}
  return pc==entry;
 }
 void VerifyBootstrap(){
  assert(pc==entry && released==1 && embeddingReads==20 && embeddingWrites==2 && embeddingReadbacks==2 && !peerWrites);
  for(unsigned i=0;i<128;i++)assert(memory.at(RB::Outer+i*4)==expected->Outer()[i]);
  for(unsigned slot=0;slot<8;slot++)for(unsigned i=0;i<1280;i++)
   assert(Get(outer[8],outer[9]+RB::MetaOffset+slot*5120+i*4)==expected->Recipe(slot)[i]);
  for(unsigned i=0;i<120*384;i++)assert(Get(outer[8],outer[9]+RB::ConvMetaOffset+i*4)==expected->Conv()[i]);
  for(unsigned i=0;i<96*192;i++)assert(Get(outer[8],outer[9]+RB::GdnMetaOffset+i*4)==expected->Gdn()[i]);
  for(unsigned i=0;i<5120;i++)assert(Get(outer[8],outer[9]+PioneerQ38GlobalFeedbackLayout::Offset+i*4)==decoded[i]);
  for(const auto& p:child)assert(!p->active && !p->sequence && p->GdnFinished(0));
  printf("PASS ACTUAL persistent ROOT bootstrap/last-vocabulary Q4_K embedding5120 bit-exact BEFORE writes/20raw reads+2writes+2readbacks/actual first-frame+all8recipes+720Conv+96GDN stage EXACT/SAME119 still virgin/no peer jobs/first returning entry reached/stack reserved%u written%u;NOT all64/multi-token/state commit/native/physical/Work/TPS\n",0x70000-minSp,0x70000-minStack);
 }
};
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 auto m=std::make_unique<TokenBootstrapMachine>(argv[1],argv[2],argv[3]);
 for(unsigned bad=0;bad<8;bad++){
  fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){
   unsigned address=m->outer[9]+TL::SessionOffset;
   if(bad==0)m->Put(m->outer[8],address,0);
   if(bad==1)m->Put(m->outer[8],address+5*4,0);
   if(bad==2)m->Put(m->outer[8],address+6*4,3);
   if(bad==3)m->Put(m->outer[8],address+8*4,m->source[0]^1);
   if(bad==4)m->Put(m->outer[8],address+24*4,7);
   if(bad==5)m->Put(m->outer[8],address+127*4,1);
   if(bad==6)m->Put(m->outer[8],m->outer[9]+TL::PromptOffset,248320);
   if(bad==7)m->Put(m->outer[8],m->outer[9]+TL::PromptOffset,17);
   assert(!m->RunBootstrap() && m->pc==0x60000 && !m->released && !m->peerWrites && !m->embeddingReads);
   unsigned packets=m->packets;for(unsigned i=0;i<2048;i++)m->StepScalar();assert(m->pc==0x60000 && m->packets==packets);
   for(const auto& p:m->child)assert(!p->active && !p->sequence && p->GdnFinished(0));_exit(0);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 puts("PASS ACTUAL persistent ROOT 8 session/prompt faults/no embedding or peer work/inert2048;NOT late-state/full-model proof");
 for(unsigned fault=1;fault<=8;fault++){
  fflush(nullptr);pid_t pid=fork();assert(pid>=0);
  if(!pid){
   m->fault=fault;
   assert(!m->RunBootstrap() && m->pc==0x60000 && m->injected && !m->released && !m->peerWrites);
   unsigned packets=m->packets;for(unsigned i=0;i<2048;i++)m->StepScalar();assert(m->pc==0x60000 && m->packets==packets);
   for(const auto& p:m->child)assert(!p->active && !p->sequence && p->GdnFinished(0));_exit(0);
  }
  int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));
 }
 puts("PASS ACTUAL persistent ROOT 8 packet/counter/source/session/header/outer/write+readback corruption faults before/after real provisional writes/no later packet or peer work/inert2048;NOT state-commit/terminal/native proof");
 assert(m->RunBootstrap());m->VerifyBootstrap();
}
