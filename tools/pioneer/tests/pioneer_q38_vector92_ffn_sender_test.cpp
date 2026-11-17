/* SPDX-License-Identifier: MIT */
// Actual vector92 matrix ROM, shared by all three scenarios below. The
// activation and weight fixture is synthetic; never reset the guest state.
#define PIONEER_Q38_VECTOR_METADATA_WORKER_MACHINE_ONLY
#include "pioneer_q38_vector_q4_worker_test.cpp"

struct VectorReusePeer:VectorMetadataPeer {
 std::array<uint32_t,2560> operand={};
 std::array<uint32_t,32> scales={};
 std::vector<uint32_t> gate;
 VectorReusePeer(const char* fixture,const char* rom,unsigned worker,unsigned groups)
  :VectorMetadataPeer(fixture,rom,12,5120,groups,worker){delay=3;}
 void Gate(){
  BootPushed();Prepare();StagePushed();AwaitPayload();FinishPushed();CheckEnd();
  for(unsigned i=0;i<operand.size();i++)operand[i]=memory.at(0xa0000+i*4);
  for(unsigned i=0;i<scales.size();i++)scales[i]=memory.at(0x94000+i*4);
  gate=Rows();CheckInput();
 }
 std::vector<uint32_t> Rows()const{return {buffers+4096,buffers+4096+FfnPersistentPeer::groups*16};}
 void CheckInput()const{
  for(unsigned i=0;i<operand.size();i++)assert(memory.at(0xa0000+i*4)==operand[i] && root.at(0xa0000+i*4)==operand[i]);
  for(unsigned i=0;i<scales.size();i++)assert(memory.at(0x94000+i*4)==scales[i] && root.at(0xab000+i*4)==scales[i]);
 }
 void StageUp(bool reuse){
  assert(GdnFinished(1) && pushes==2 && !matrixScope && !pending && inputReads==2);
  ClearTensorState();
  std::array<uint32_t,8> actual;
  for(unsigned i=0;i<8;i++)actual[i]=Count(i);
  for(unsigned i=0;i<8;i++)countBase[i]=actual[i];
  NextPackets();
  for(unsigned i=0;i<8;i++)assert(Count(i)==actual[i]);
  stageCalls=waitCalls=restoreCommands=rawWrites=0;validatedTile=UINT32_MAX;assert(!stageReturn);
  mailbox.fill(0);payload.fill(0);pushes=programmed=0;pushCommand=false;pushReadback.fill(false);
  issued.clear();
  constexpr unsigned gap=0x04000000;
  // Same activations, distinct Q4 values and address range for up. The
  // real vector metadata observer and independent row checks remain active.
  for(auto& t:tiles)for(unsigned i=16*16;i<t.size();i++)t[i]^=0x11;
  for(auto& p:packets)if(p.tile>=0)p.address+=gap;
  base+=gap;
  if(reuse){packets.erase(packets.begin(),packets.begin()+inputReads);inputReads=0;}
  for(unsigned i=0;i<4096;i++)buffers[4096+i]=0x7fc00000;
  Prepare();CheckInput();
  memory[M::Job+4]=2;memory[M::Job+7*4]=2026;
  memory[M::Job+22*4]=base;memory[M::Job+25*4]=reuse?0:1;
  memory[M::Doorbell]=2;sequence=2;
 }
 void Up(){
  while(!heldPayload && !GdnFaulted())StepBlock();
  assert(heldPayload && pushes==1 && !mailbox[0] && memory.at(M::Status+4)==1);
  const auto sealed=mailbox;
  for(unsigned i=0;i<2048;i++)StepBlock();
  assert(heldPayload && pushes==1 && mailbox==sealed && !GdnFaulted());
  AckPayload();while(!GdnFinished(2) && !GdnFaulted())StepBlock();
  const std::array<uint32_t,4> slave={{4,10,4,4}};
  assert(pc==waitPC && !heldPayload && !pending && pushes==2 && !programmed
   && matrixInstructions==ExpectedMatrixInstructions() && requests==ExpectedReadPackets() && responses==requests
   && rootMaster==rootOriginal && rootSlave==slave && CorrectRows());
  std::array<uint32_t,64> job;for(unsigned i=0;i<64;i++)job[i]=memory.at(M::Job+i*4);
  assert(Push::Receipt(mailbox.data(),boot.data(),job.data(),beforePush.data()));
  CheckEnd();CheckInput();assert(Rows()!=gate);
 }
 void Refused(){
  while(!GdnFaulted() && !heldPayload)StepBlock();
  assert(GdnFaulted() && !requests && !responses && !matrixInstructions && !pushes && !heldPayload && !mailbox[0]);
 }
};

int main(int argc,char** argv){
 assert(argc==4);setvbuf(stdout,nullptr,_IOLBF,0);
 (void)&RunPersistentGdnPair;(void)&Compare;(void)&Round;
 for(unsigned w:{1U,69U,119U}){
  const unsigned groups=w==1?10:9;
  auto original=std::make_unique<VectorReusePeer>(argv[1],argv[2],w,groups);
  auto refresh=std::make_unique<VectorReusePeer>(argv[1],argv[3],w,groups);
  auto retained=std::make_unique<VectorReusePeer>(argv[1],argv[3],w,groups);
  for(auto* peer:{original.get(),refresh.get(),retained.get()}){
   peer->Gate();assert(peer->gate==original->gate);
   peer->StageUp(peer==retained.get());peer->Up();
  }
  assert(original->Rows()==refresh->Rows() && retained->Rows()==refresh->Rows());
  assert(original->issued==refresh->issued && retained->issued==refresh->issued);
  assert(retained->metadataBlocks==2*groups*20 && retained->metadataBlocks==refresh->metadataBlocks);
  assert(original->requests==refresh->requests && retained->requests+2==refresh->requests);
  assert(retained->beforePush[1]+162==refresh->beforePush[1]);
  printf("PASS vector92 worker%u gate/up rows%u bit-identical; read packets %u->%u, minus 162 64B flits\n",
   w,groups*16,refresh->requests,retained->requests);
 }
 // The exact old sender cannot accept the new descriptor even with a valid
 // preceding job; the new sender rejects its use before any cold fill.
 for(unsigned which=0;which<7;which++){
  auto p=std::make_unique<VectorReusePeer>(argv[1],argv[which==0?2:3],69,1);
  if(which==1){p->BootPushed();p->Prepare();p->StagePushed();p->memory[M::Job+25*4]=0;}
  else{
   p->Gate();p->StageUp(true);
   if(which==2)p->memory[M::Job+7*4]=2027;
   if(which==3)p->memory[M::Job+5*4]=1;
   if(which==4)p->memory[M::Job+6*4]=1;
   if(which==5)p->memory[0xa0000]=0x7fc00000;
   if(which==6)p->memory[0x94000]=0x7fc00000;
  }
  p->Refused();
 }
 puts("PASS exact vector92 sender; 7 refusals before new NoC/tensor/receipt; synthetic only, no timing");
}
