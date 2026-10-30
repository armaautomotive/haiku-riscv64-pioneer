/* SPDX-License-Identifier: MIT */
// Actual QGW3 instructions through the SAME GlobalChainPeer endpoint used
// by the original-model runs. Synthetic raw inputs only; never GPU authority.
#define PIONEER_Q38_GLOBAL_CHAIN_MACHINE_ONLY
#include "pioneer_q38_global_chain_instruction_test.cpp"
struct PushedFormatPeer:GlobalChainPeer {
 using GlobalChainPeer::GlobalChainPeer;
 unsigned previousPC=0;
 void StepBlock()override{previousPC=pc;GlobalChainPeer::StepBlock();}
 uint32_t Load(uint32_t a)override{
  try{return GlobalChainPeer::Load(a);}catch(const std::out_of_range&){
   fprintf(stderr,"FORMAT_ENDPOINT missing raw load address%08x PC%08x\n",a,pc);throw;
  }
 }
};
struct PushedFormatBytes:CohortBytes {
 const unsigned route,base,tileBytes,worker;
 const std::vector<std::vector<uint8_t>> tiles;
 const std::map<uint32_t,uint32_t> input;
 std::array<uint32_t,64> mailbox={};
 std::array<uint32_t,4> slave={};unsigned pushes=0;
 explicit PushedFormatBytes(const GlobalChainPeer& p):route(p.matrixJob[8]),base(p.base),
  tileBytes(unsigned(p.tile.size())),worker(p.logicalWorker),tiles(p.tiles),input(p.root){}
 uint32_t Get(unsigned r,unsigned a)const override{
  assert(r==route && a>=base && !(a&3));unsigned offset=a-base;
  return Word(tiles.at(offset/tileBytes),offset%tileBytes);
 }
 void Put(unsigned,unsigned,uint32_t)override{assert(false);}
 uint64_t Clock()const override{assert(false);return 0;}
 uint32_t RootWord(unsigned a)const override{
  if(!input.count(a))fprintf(stderr,"FORMAT_ENDPOINT missing raw ROOT word%08x\n",a);
  return input.at(a);
 }
 static void PushBytes(void* context,unsigned w,const uint32_t* command,const uint32_t* data){
  auto& b=*static_cast<PushedFormatBytes*>(context);assert(w==b.worker && b.pushes<2);
  assert(command[0]==G::PushBuffer && !command[1] && command[2]==G::Peer(w)
   && command[3]==G::Slot(w) && !command[4] && command[5]==G::Root && !command[6]
   && command[7]==0x212 && command[8]==(b.pushes?4U:256U) && !command[9]);
  if(!b.pushes){assert(!data[0] && !b.mailbox[0]);for(unsigned i=0;i<64;i++)b.mailbox[i]=data[i];}
  else{assert(!b.mailbox[0] && data[0]==G::ReceiptMagic);b.mailbox[0]=data[0];}
  b.slave[0]++;b.slave[1]+=(command[8]+63)/64;b.slave[2]++;b.slave[3]++;b.pushes++;
 }
};
static void Run(const char* old,const char* image,unsigned type,unsigned k,unsigned groups,unsigned worker,bool missingActivation=false){
 printf("START actual G3 endpoint type%u K%u groups%u worker%u\n",type,k,groups,worker);
 auto p=std::make_unique<PushedFormatPeer>(old,image,worker,type,k,groups);
 PushedFormatBytes bytes(*p);p->bytes=&bytes;p->pushedContext=&bytes;p->pushedBytes=PushedFormatBytes::PushBytes;
 for(unsigned i=0;i<64;i++)p->memory[M::Boot+i*4]=p->boot[i];
 assert(p->BootGlobal() && !p->sequence && !p->requests && !p->matrixInstructions);
 std::array<uint32_t,64> job={};const unsigned head[]={G::Magic,1,1,worker,p->selfRoute,0,0,2025};
 for(unsigned i=0;i<8;i++){job[i]=head[i];job[8+i]=p->boot[8+i];}
 for(unsigned i=0;i<10;i++)job[16+i]=p->matrixJob[i];job[26]=1;
 assert(G::JobShape(job.data(),p->boot.data(),0,0));
 for(unsigned i=0;i<64;i++)p->memory[M::Job+i*4]=job[i];p->memory[M::Doorbell]=1;p->Receive();
 while(!p->GdnFinished(1) && !p->GdnFaulted())p->StepBlock();
 if(missingActivation){
  assert(type==12 && k==17408 && worker<=68 && p->GdnFaulted() && p->memory.at(M::Status+8)==G::Fault
   && !p->memory.at(M::Status+4) && !p->pending && !p->requests && !p->responses
   && !p->matrixInstructions && !p->pushPackets && !bytes.pushes && !bytes.mailbox[0]);
  const auto saved=p->memory;const auto mailbox=bytes.mailbox;const auto slave=bytes.slave;
  std::array<uint32_t,10> command;std::array<uint32_t,8> counts;
  for(unsigned i=0;i<10;i++)command[i]=p->command[i];for(unsigned i=0;i<8;i++)counts[i]=p->Count(i);
  for(unsigned i=0;i<2048;i++)p->StepBlock();
  assert(p->GdnFaulted() && p->memory==saved && bytes.mailbox==mailbox && bytes.slave==slave
   && !p->requests && !p->responses && !p->matrixInstructions && !p->pushPackets && !bytes.pushes);
  for(unsigned i=0;i<10;i++)assert(command[i]==p->command[i]);for(unsigned i=0;i<8;i++)assert(counts[i]==p->Count(i));
  puts("PASS actual G3 private low-rank down-job guard rejects missing activation BEFORE any packet/tensor/push/no retry/cleanup/inert2048;NOT an admitted long-Q4 job");
  return;
 }
 const unsigned expectedPhases=type==12?2:4;
 const unsigned expectedReads=(k*2+16383)/16384+1+groups*(k/(type==8?32:256));
 const std::array<uint32_t,4> expectedSlave={{2,5,2,2}};
 const bool okay=!p->GdnFaulted() && !p->memory.at(M::Status+8) && !p->pending && p->drained
  && p->minStack>=0x6e000 && p->MatrixPhasesPerPanel()==expectedPhases
  && p->matrixInstructions==expectedPhases*groups*(k/16) && p->requests==expectedReads
  && p->responses==expectedReads && p->pushPackets==2 && !p->pushWords
  && bytes.pushes==2 && bytes.slave==expectedSlave && !p->dotProbeReads && p->CorrectRows();
 if(!okay)fprintf(stderr,"FORMAT_ENDPOINT failed type%u K%u groups%u PC%08x previousPC%08x fault%08x pending%u drained%u minStack%08x phases%u tensor%u/%u reads%u/%u responses%u pushPackets%u pushWords%u writes%u slave%u,%u,%u,%u dotReads%u\n",
  type,k,groups,p->pc,p->previousPC,p->memory.at(M::Status+8),unsigned(p->pending),unsigned(p->drained),p->minStack,
  p->MatrixPhasesPerPanel(),p->matrixInstructions,expectedPhases*groups*(k/16),p->requests,expectedReads,p->responses,
  p->pushPackets,p->pushWords,bytes.pushes,bytes.slave[0],bytes.slave[1],bytes.slave[2],bytes.slave[3],p->dotProbeReads);
 assert(okay);
 std::array<uint32_t,8> before;for(unsigned i=0;i<8;i++)before[i]=bytes.mailbox[56+i];
 assert(G::Receipt(bytes.mailbox.data(),p->boot.data(),job.data(),before.data()));
 const unsigned delta[]={0,0,2,0,2,5,2,2};
 for(unsigned i=0;i<8;i++)assert(p->Count(i)==before[i]+delta[i]);
 for(unsigned i=0;i<64;i++){
  auto changed=bytes.mailbox;changed[i]^=1U<<((i*7+5)%32);
  assert(!G::Receipt(changed.data(),p->boot.data(),job.data(),before.data()));
 }
 printf("PASS SAME GlobalChainPeer QGW3 type%u K%u groups%u worker%u/%u phases/%u actual tensor instructions/%u actual reads/actual payload+commit/exact FP32 rows/64 receipt mutations;SYNTHETIC raw endpoints,NOT full model/native/physical/Work/TPS\n",
  type,k,groups,worker,expectedPhases,p->matrixInstructions,p->requests);
}
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Refusals;(void)&LateRefusals;
 unsigned cases=0;
 for(unsigned type:{8U,12U,14U}){
  for(auto shape:std::array<std::pair<unsigned,unsigned>,4>{{{5120,1},{6144,3},{17408,1},{5120,16}}}){
   // Low ranks1..68 genuinely require an earlier returned activation for
   // K17408/Q4. Do not forge that private readiness; use a legal high rank
   // for this isolated positive case, and exercise missing readiness below.
   const unsigned worker=type==12 && shape.first==17408?119:1+(cases%3==1?95:cases%3==2?118:0);
   Run(argv[1],argv[2],type,shape.first,shape.second,worker);cases++;
  }
 }
 Run(argv[1],argv[2],12,17408,1,1,true);
 printf("PASS %u actual G3 matrix+completion cases across all3 formats/K5120,6144,17408/groups1,3,16;NO supplied products or full-model/silicon speed claim\n",cases);
}
