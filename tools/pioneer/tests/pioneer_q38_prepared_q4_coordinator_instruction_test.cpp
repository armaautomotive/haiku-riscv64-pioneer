/* SPDX-License-Identifier: MIT */
// Actual linked ROOT functions -> actual worker descriptor read -> tensor
// instructions -> ACK-gated completion. Synthetic raw bytes/ABI arguments
// and NoC only. NOT a whole ROOT boot, native owner, model or speed result.
#define PIONEER_Q38_PREPARED_WORKER_MACHINE_ONLY
#include "pioneer_q38_prepared_exact_q4_worker_test.cpp"
#include "../PioneerQ38PreparedQ4RootLayout.h"
#include "../PioneerQ38MixedRecurrentBlockRootLayout.h"
using PR=PioneerQ38PreparedQ4RootLayout;
namespace RR=PioneerQ38MixedRecurrentBlockRootLayout;
namespace RP=PioneerQ38MixedRecurrentBlockProjectionLayout;
struct PreparedRootImage {
 struct Section{unsigned base;std::vector<uint8_t> bytes;};
 std::vector<Section> sections;
 unsigned stage=0,bind=0,read=0,seal=0,matrix=0,transfer=0,faultLoop=0,receipt=0,packetSeal=0;
 explicit PreparedRootImage(const char* path){
  for(auto item:{std::make_pair("text",0x40000U),{"terminal",0x60000U},{"conv",0x64000U},{"prepared",0x6c000U}})
   sections.push_back({item.second,Read((std::string(path)+"/"+item.first+".bin").c_str())});
  std::ifstream symbols(std::string(path)+"/program.symbols");std::string at,kind,name;
  std::vector<unsigned> seals,matrices,transfers;
  while(symbols>>at>>kind>>name){
   const auto a=unsigned(std::stoul(at,nullptr,16));
   if(name=="q38_prepared_q4_stage_shared")stage=a;
   if(name=="q38_prepared_q4_bind_shared")bind=a;
   if(name=="q38_prepared_q4_read_shared")read=a;
   if(name=="q38_token_fault_report")faultLoop=a+28;
   if(name=="q38_pushed_receipt_shared")receipt=a;
   if(name=="q38_layer_return_recurrent_packet_seal")packetSeal=a;
   if(name=="_ZN18DensePartitionRootL4SealEPKj")seals.push_back(a);
   if(name=="_ZN10ProjectionL9MatrixJobEPKjjj")matrices.push_back(a);
   if(name=="_ZN18DensePartitionRootL8TransferEPKjPjjjjb")transfers.push_back(a);
  }
  // The first component is recurrent. Never replace its compiled seal with
  // a host stub; both components' distinct entry identities are retained.
  assert(seals.size()==2 && matrices.size()==2 && transfers.size()==2);
  seal=seals[0];matrix=matrices[0];transfer=transfers[0];assert(stage && bind && read && faultLoop && receipt && packetSeal);
 }
 const Section* Find(unsigned a)const{for(const auto& s:sections)if(a>=s.base && a-s.base<s.bytes.size())return &s;return nullptr;}
};
struct PreparedRootMachine:Machine {
 enum:unsigned{Owner=0x6e800,Frame=Owner+768,Tracked=0x6e400,Expected=0x6e440,Return=0x60004};
 const PreparedRootImage& image;std::map<unsigned,unsigned> memory,source;
 std::array<unsigned,10> command={};std::array<bool,10> readback={};
 std::array<unsigned,8> counters={};
 unsigned packets=0,responses=0,polls=0,faultAt=0,counterFault=0,afterPacketMutation=0,minimumStack=0x70000,stores=0;
 unsigned commandMutationWord=UINT32_MAX,commandReadFaultWord=UINT32_MAX;
 unsigned layer,role,remote,workerJobWrites=0,workerDoorbells=0,faultWrites=0,incomingPayload=0,incomingCommit=0;
 bool pending=false;PreparedQ4Peer* endpoint=nullptr;
 PreparedRootMachine(const PreparedRootImage& rom,const PreparedQ4Peer& peer,unsigned l,unsigned r):image(rom),layer(l),role(r){
  for(unsigned i=0;i<64;i++){
   memory[Owner+i*4]=memory[RR::Boot+i*4]=0x31400000+i*0x129;
   memory[Frame+i*4]=memory[RR::Header+i*4]=0;
  }
  for(unsigned i=0;i<128;i++)memory[Owner+256+i*4]=memory[RR::Outer+i*4]=0x12340000+i*0x117;
  memory[Owner+256+48*4]=memory[RR::Outer+48*4]=memory[RR::Doorbell]=7;
  memory[Owner+1024]=r==2?10:r==3?11:13;
  auto frame=[&](unsigned i,unsigned v){memory[Frame+i*4]=memory[RR::Header+i*4]=v;};
  frame(0,PioneerQ38MixedProjectionRootLayout::Magic);frame(4,0x04000000);
  frame(6,2025);frame(8,PioneerQ38ProjectionRoute(0));frame(9,129);frame(14,peer.inputs);
  frame(15,12);frame(40,0);frame(42,l);frame(43,r);frame(48,7);
  for(unsigned i=0;i<16;i++){
   const unsigned v=peer.source[i];frame(16+i,v);memory[Owner+(16+i)*4]=memory[RR::Boot+(16+i)*4]=v;
   memory[0xffb20000U+BlackholeDramSourceOffset(i)]=v;
  }
  for(unsigned i=0;i<8;i++){
   const unsigned v=peer.boot[8+i];frame(32+i,v);memory[Owner+(8+i)*4]=memory[RR::Boot+(8+i)*4]=v;
   memory[Tracked+i*4]=counters[i]=0xfffffff8;
  }
  for(unsigned i=0;i<4;i++)memory[0xffb20040U+i*0x800]=0;
  memory[0xffb20064U]=0x10101010;memory[0xffb20068U]=0;
  for(unsigned i=0;i<32;i++)memory[0xffb20240U+i*4]=0;
  for(unsigned i=0;i<16;i++)memory[RP::MatrixHeader+i*4]=memory[RP::HeaderShadow+i*4]=i;
  for(unsigned i=0;i<8;i++)memory[RP::Banks+i*4]=memory[RP::BankShadow+i*4]=i;
  for(unsigned i=0;i<64;i++)memory[RP::Summary+i*4]=0;
  remote=0x04000000+0x1000000+l*0x80000+PR::PhaseOffset(r);
  const unsigned header[]={PR::Magic,1,l,r,peer.inputs,119,16,9600,PR::PhaseOffset(r),PR::RootTable,PR::RootLedger,1,0,0,0,0};
  for(unsigned i=0;i<PR::Bytes/4;i++)source[remote+i*4]=0;
  for(unsigned i=0;i<16;i++)source[remote+i*4]=header[i];
  unsigned reads=0,flits=0;for(const auto& p:peer.packets)if(p.tile<-1){reads++;flits+=p.bytes/64;}
  for(unsigned w=0;w<119;w++){
   for(unsigned i=0;i<16;i++)source[remote+64+w*64+i*4]=i==1?w+1:i==2?0:peer.descriptor[i];
   const unsigned ledger[]={reads,flits,w*16,0};
   for(unsigned i=0;i<4;i++)source[remote+PR::LedgerOffset+w*16+i*4]=ledger[i];
   for(unsigned i=0;i<8;i++)memory[RP::Parts+w*32+i*4]=memory[RP::PartsShadow+w*32+i*4]=
    i==0?peer.base:i==2?peer.FfnPersistentPeer::groups:i==4?peer.sourceRoute:0;
   memory[RP::Rows+w*4]=memory[RP::RowsShadow+w*4]=w*16;
  }
  pc=0;regs[2]=0x70000;regs[27]=Owner;
 }
 static unsigned Counter(unsigned i){const unsigned a[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};return a[i];}
 bool CodeAddress(unsigned a)const override{return image.Find(a);}
 unsigned Index(unsigned a)override{const auto* s=image.Find(a);assert(s && !(a&3));l1[8180]=Word(s->bytes,a-s->base);return 8180;}
 unsigned StepLimit()const override{return 18000000;}
 unsigned Load(unsigned a)override{
  assert(!(a&3));if(CodeAddress(a))return l1[Index(a)];
  if(a>=0xffb20000U && a<0xffb20028U){unsigned i=(a-0xffb20000U)/4;readback[i]=true;return command[i]^(i==commandReadFaultWord?1U:0U);}
  if(a==0xffb20040U){if(pending && ++polls>3 && packets!=faultAt)Complete();return unsigned(pending);}
  for(unsigned i=0;i<8;i++)if(a==0xffb20000U+Counter(i))return counters[i];
  if(!memory.count(a))fprintf(stderr,"PREPARED_ROOT_UNMAPPED %08x pc%08x\n",a,pc);
  return memory.at(a);
 }
 void Complete(){
  const bool write=command[7]==0x212;const unsigned bytes=command[8];
  if(!write)for(unsigned at=0;at<bytes;at+=4)memory[command[3]+at]=source.at(command[0]+at);
  else{
   assert(endpoint && command[2]==129 && command[5]==M::Peer(endpoint->logicalWorker));
   assert(endpoint->pc==endpoint->waitPC && !endpoint->sequence && !endpoint->requests);
   if(command[3]==M::Job){
    assert(bytes==256 && !workerJobWrites && !workerDoorbells);
    for(unsigned i=0;i<64;i++)endpoint->memory[M::Job+i*4]=memory.at(command[0]+i*4);
    std::array<unsigned,64> j;for(unsigned i=0;i<64;i++)j[i]=endpoint->memory.at(M::Job+i*4);
    assert(Push::JobShape(j.data(),endpoint->boot.data(),0,0));workerJobWrites++;
   }else{
    assert(command[3]==M::Doorbell && bytes==4 && workerJobWrites==1 && !workerDoorbells);
    endpoint->sequence=memory.at(command[0]);assert(endpoint->sequence==1);
    endpoint->memory[M::Doorbell]=endpoint->sequence;workerDoorbells++;
   }
  }
  for(unsigned i=0;i<8;i++)counters[i]+=i==(write?5U:1U)?(bytes+63)/64:
   i==2 || i==6 || i==(write?4U:0U) || i==(write?7U:3U)?1U:0U;
  if(packets==counterFault)counters[3]++;
  if(commandMutationWord<10)command[commandMutationWord]^=1;
  if(afterPacketMutation)memory.at(afterPacketMutation)^=1;
  pending=false;responses++;
 }
 void Store(unsigned a,unsigned v)override{
  assert(!(a&3));stores++;
  if(a>=0xffb20000U && a<0xffb20028U){assert(!pending);unsigned i=(a-0xffb20000U)/4;command[i]=v;readback[i]=false;return;}
  if(a==0xffb20040U){
   assert(v==1 && !pending);for(bool b:readback)assert(b);
   assert(!command[1] && !command[4] && command[5] && !command[6] && !command[9]);
   if(!command[7]){
    assert(command[2]==PioneerQ38ProjectionRoute(0) && command[3]==RP::Scratch && command[5]==129);
    assert((command[0]==remote && command[8]==64) || (command[0]==remote+64 && command[8]==PR::RootTableBytes));
   }else assert(command[7]==0x212 && command[0]>=RP::Scratch && command[0]<RP::Scratch+32);
   pending=true;polls=0;packets++;return;
  }
  if(a==0x75460 || a==0x75448){
   assert((a==0x75460 && pc==image.faultLoop-24 && !faultWrites)
    || (a==0x75448 && pc==image.faultLoop-8 && faultWrites==1 && v==0x51465431));
   memory[a]=v;faultWrites++;return;
  }
  const bool stack=a>=0x6f000 && a<0x70000;
  if(!(stack || (a>=Tracked && a<Tracked+32) || (a>=PR::RootTable && a<PR::RootEnd)
   || (a>=RP::Scratch && a<RP::Scratch+16384) || (a>=RP::Summary && a<RP::Summary+256)))
   fprintf(stderr,"PREPARED_ROOT_FORBIDDEN_STORE %08x=%08x pc%08x\n",a,v,pc);
  assert(stack || (a>=Tracked && a<Tracked+32) || (a>=PR::RootTable && a<PR::RootEnd)
   || (a>=RP::Scratch && a<RP::Scratch+16384) || (a>=RP::Summary && a<RP::Summary+256));
  if(stack)minimumStack=std::min(minimumStack,a);memory[a]=v;
 }
 bool Call(unsigned entry,std::initializer_list<unsigned> args){
  assert(!pc || pc==Return);pc=entry;regs[1]=Return;unsigned reg=10;for(unsigned v:args)regs[reg++]=v;
  while(pc!=Return && pc!=image.faultLoop)Step();const bool okay=pc==Return;
  if(okay){assert(regs[2]==0x70000 && regs[27]==Owner);for(unsigned i=0;i<8;i++)assert(memory.at(Tracked+i*4)==counters[i]);}
  else assert(faultWrites==2 && memory.at(0x75448)==0x51465431);
  const auto before=memory;const auto cmds=command;const auto counts=counters;const unsigned sent=packets,written=stores,terminal=pc;
  for(unsigned i=0;i<2048;i++)Step();
  assert(pc==terminal && memory==before && command==cmds && counters==counts && packets==sent && stores==written);
  return okay;
 }
 bool Stage(){return Call(image.stage,{Frame,Tracked,image.seal});}
 bool Job(unsigned w,unsigned request){return Call(image.matrix,{Frame,w,request});}
 bool Send(unsigned w,unsigned address,unsigned bytes){return Call(image.transfer,{Frame,Tracked,w,address,bytes,1});}
 bool Receipt(unsigned w){return Call(image.receipt,{Frame,w,Expected,image.packetSeal});}
 void AdmitEndpoint(const PreparedQ4Peer& p){
  // Raw observations at this function-test ABI boundary, NOT execution of
  // whole-ROOT Admit. Retain the cold real worker boot/counter values.
  const unsigned w=p.logicalWorker-1,blocks=p.inputs/256,groups=p.FfnPersistentPeer::groups;
  const unsigned originalReads=groups*blocks/2+(p.inputs*2+16383)/16384+1;
  const unsigned originalFlits=groups*blocks*2304/64+(p.inputs*2+((blocks*4+63)&~63U))/64;
  const unsigned reads=originalReads+source.at(remote+PR::LedgerOffset+w*16);
  const unsigned flits=originalFlits+source.at(remote+PR::LedgerOffset+w*16+4);
  for(unsigned i=0;i<64;i++)memory[RP::PeerBoots+w*256+i*4]=p.memory.at(M::Boot+i*4);
  for(unsigned i=0;i<8;i++){
   const unsigned before=p.Count(i),delta=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0;
   const unsigned pushed=i==5?5U:i==2 || i==4 || i==6 || i==7?2U:0;
   memory[RP::Counters+w*32+i*4]=before;memory[Expected+i*4]=before+delta+pushed;
  }
  for(unsigned i=0;i<64;i++)memory[Push::Slot(p.logicalWorker)+i*4]=0;
 }
 void Incoming(const PreparedQ4Peer& p,unsigned bytes){
  // Only emitted outgoing packets may populate the ROOT receipt. Never
  // copy a host-built completion or derive the expected counts from it.
  assert(!p.command[1] && p.command[2]==p.selfRoute && p.command[3]==Push::Slot(p.logicalWorker)
   && !p.command[4] && p.command[5]==129 && !p.command[6] && p.command[7]==0x212
   && p.command[8]==bytes && !p.command[9] && p.command[0]==Push::PushBuffer);
  assert(bytes==256?(!incomingPayload && !incomingCommit):(bytes==4 && incomingPayload==1 && !incomingCommit));
  for(unsigned i=0;i<bytes/4;i++)memory[Push::Slot(p.logicalWorker)+i*4]=p.memory.at(Push::PushBuffer+i*4);
  if(bytes==256)incomingPayload++;else incomingCommit++;
 }
};
struct CoordinatorPeer:PreparedQ4Peer {
 PreparedRootMachine* coordinator=nullptr;mutable unsigned rootDescriptorWords=0;
 CoordinatorPeer(const char* fixture,const char* image,unsigned k,unsigned prefix,unsigned worker)
  :PreparedQ4Peer(fixture,image,12,k,1,prefix,worker,PioneerQ38ProjectionRoute(0)){}
 unsigned PreparedSourceWord(const Packet& p,unsigned at)const override{
  if(p.tile!=-2)return PreparedQ4Peer::PreparedSourceWord(p,at);
  assert(coordinator && p.route==129 && p.address==PreparedLayout::RootSlot(logicalWorker));rootDescriptorWords++;
  return coordinator->memory.at(p.address+at);
 }
 void Store(unsigned a,unsigned value)override{
  const bool send=a==0xffb20040 && command[7]==0x212;
  const unsigned bytes=send?command[8]:0;PreparedQ4Peer::Store(a,value);
  if(send){assert(coordinator);coordinator->Incoming(*this,bytes);}
 }
};
int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 PreparedRootImage image(argv[1]);unsigned stamps=0,refusals=0,workers=0,stack=0;
 auto fixture=std::make_unique<CoordinatorPeer>(argv[2],argv[3],5120,13,1);
 for(unsigned layer:{0U,63U})for(unsigned role:{2U,3U,4U}){
  auto shape=role==4?std::make_unique<CoordinatorPeer>(argv[2],argv[3],17408,13,1):nullptr;
  const auto& p=shape?*shape:*fixture;auto root=std::make_unique<PreparedRootMachine>(image,p,layer,role);
  assert(root->Stage() && root->packets==2 && root->responses==2);
  for(unsigned w=0;w<119;w++){
   assert(root->Job(w,17));const unsigned slot=PR::RootTable+w*64;
   for(unsigned i=0;i<16;i++)assert(root->memory.at(slot+i*4)==(i==1?w+1:i==2?17:p.descriptor[i]));
   assert(root->Job(w,17));stamps+=2;
  }
  assert(root->packets==2);stack=std::max(stack,0x70000-root->minimumStack);
  printf("PASS_PREPARED_COORDINATOR_STAGE layer%u role%u actualReads2 bytes9600 actualMatrixJobs238 retainedPhaseShadow1\n",layer,role);
 }
 // Staged data may not change between the phase capture and dispatch.
 for(unsigned region:{unsigned(PR::RootHeader),unsigned(PR::RootTable),unsigned(PR::RootLedger)})
  for(unsigned i=0;i<(region==PR::RootLedger?4U:16U);i++){
   auto root=std::make_unique<PreparedRootMachine>(image,*fixture,0,2);assert(root->Stage());
   // The request alone may already contain the SAME legitimate stamp
   // (Gather rebuilds it). Use a different request for this refusal.
   root->memory.at(region+i*4)^=region==PR::RootTable && i==2?2U:1U;
   assert(!root->Job(0,1) && root->packets==2);refusals++;
  }
 {
  auto root=std::make_unique<PreparedRootMachine>(image,*fixture,0,2);assert(root->Stage() && root->Job(0,1));
  assert(!root->Job(0,2) && root->memory.at(PR::RootTable+8)==1 && root->packets==2);refusals++;
 }
 for(unsigned position:{1U,2U})for(bool wrongCounter:{false,true}){
  auto root=std::make_unique<PreparedRootMachine>(image,*fixture,0,2);
  if(wrongCounter)root->counterFault=position;else root->faultAt=position;
  assert(!root->Stage() && root->packets==position && root->responses==(wrongCounter?position:position-1));
  for(unsigned i=0;i<8;i++)assert(root->memory.at(PreparedRootMachine::Tracked+i*4)==0xfffffff8+(position-1)*(i==1?1U:i==0 || i==2 || i==3 || i==6?1U:0U));
  refusals++;
 }
 for(unsigned address:{unsigned(RR::Boot),unsigned(RR::Header),unsigned(RR::Outer),unsigned(RR::Doorbell),0xffb20100U}){
  auto root=std::make_unique<PreparedRootMachine>(image,*fixture,0,2);root->afterPacketMutation=address;
  assert(!root->Stage() && root->packets==1 && root->responses==1);
  for(unsigned i=0;i<8;i++)assert(root->memory.at(PreparedRootMachine::Tracked+i*4)==0xfffffff8);refusals++;
 }
 for(unsigned i=0;i<16;i++){
  auto root=std::make_unique<PreparedRootMachine>(image,*fixture,0,2);root->source.at(root->remote+i*4)^=1;
  assert(!root->Stage() && root->packets==1 && root->responses==1);refusals++;
 }
 for(unsigned word=0;word<10;word++)for(bool after:{false,true}){
  auto root=std::make_unique<PreparedRootMachine>(image,*fixture,0,2);
  if(after)root->commandMutationWord=word;else root->commandReadFaultWord=word;
  assert(!root->Stage() && root->packets==unsigned(after) && root->responses==unsigned(after));
  for(unsigned i=0;i<8;i++)assert(root->memory.at(PreparedRootMachine::Tracked+i*4)==0xfffffff8);refusals++;
 }
 for(unsigned worker:{1U,96U,119U}){
  const unsigned k=worker==119?17408:5120,role=k==17408?4:2;
  auto p=std::make_unique<CoordinatorPeer>(argv[2],argv[3],k,worker==1?0:13,worker);p->delay=3;p->BootPushed();p->Prepare();
  auto root=std::make_unique<PreparedRootMachine>(image,*p,0,role);p->coordinator=root.get();root->endpoint=p.get();
  root->AdmitEndpoint(*p);
  assert(root->Stage() && root->Job(worker-1,1) && root->Send(worker-1,M::Job,256));
  // A raw readback of the actual emitted job is the retained receipt input.
  for(unsigned i=0;i<64;i++)root->memory[RP::PeerJobs+(worker-1)*256+i*4]=p->memory.at(M::Job+i*4);
  root->memory[RP::Nonces+(worker-1)*4]=1;
  root->memory[RP::Scratch]=1;assert(root->Send(worker-1,M::Doorbell,4));
  p->AwaitPayload();
  assert(root->incomingPayload==1 && !root->incomingCommit && !root->memory.at(Push::Slot(worker)));
  p->FinishPushed();p->CheckEnd();
  assert(root->incomingPayload==1 && root->incomingCommit==1 && root->Receipt(worker-1));
  for(unsigned word:{0U,1U,2U,3U,8U,9U,10U,11U,39U,55U,56U,57U,58U,59U,60U,61U,62U,63U}){
   fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
   if(!pid){root->memory.at(Push::Slot(worker)+word*4)^=1;assert(!root->Receipt(worker-1));_exit(0);}
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));refusals++;
  }
  for(unsigned i=0;i<8;i++){
   fflush(nullptr);const pid_t pid=fork();assert(pid>=0);
   if(!pid){root->memory.at(PreparedRootMachine::Expected+i*4)^=1;assert(!root->Receipt(worker-1));_exit(0);}
   int status=0;assert(waitpid(pid,&status,0)==pid && WIFEXITED(status) && !WEXITSTATUS(status));refusals++;
  }
  p->StopLocal();
  assert(root->workerJobWrites==1 && root->workerDoorbells==1 && root->packets==4 && p->rootDescriptorWords==16);
  assert(p->extraReads==root->memory.at(PR::RootLedger+(worker-1)*16)
   && p->extraFlits==root->memory.at(PR::RootLedger+(worker-1)*16+4));
  stack=std::max(stack,0x70000-std::min(root->minimumStack,p->minStack));workers++;
  printf("PASS_PREPARED_COORDINATOR_WORKER worker%u K%u actualROOTJobAndDoorbell2 actualROOTDescriptorWords16 actualTensorInstructions%u ACKPayloadThenCommit1 actualROOTReceiptAndIndependentCounterFrontier1 actualSTOP1\n",worker,k,p->matrixInstructions);
 }
 printf("PASS_PREPARED_COORDINATOR_INSTRUCTIONS actualMatrixJobs%u refusals%u coupledVirginWorkers%u maxStack%u/8192;FUNCTION_ABI_SYNTHETIC_SOURCE_NOT_WHOLE_ROOT_NATIVE_MODEL_CARD_OR_TPS\n",stamps,refusals,workers,stack);
}
