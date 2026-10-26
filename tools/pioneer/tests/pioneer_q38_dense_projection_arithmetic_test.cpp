/* SPDX-License-Identifier: MIT */
// ACTUAL QDM1 ROOT + ACTUAL matrix peer instructions. The fabric copies
// bytes, never manufactures products, status, proofs or private sequences.
// Synthetic learned/input bytes; NOT native ownership or silicon timing.
#define PIONEER_Q38_RESIDENT_OPERATOR_SERVICE_MACHINE_ONLY
#include "pioneer_q38_resident_operator_service_image_test.cpp"
#define PIONEER_Q38_DISPATCH_BASE_ALREADY_INCLUDED
#define PIONEER_Q38_PROJECTION_MATRIX_MACHINE_ONLY
#include "pioneer_q38_projection_matrix_image_test.cpp"
struct DenseArithmeticPeer:ResidentOperatorPeer {
 DenseArithmeticPeer(const char* image,unsigned type,unsigned inputs,unsigned groups,unsigned worker,unsigned first)
  :ResidentOperatorPeer(image,worker,type,inputs,groups){
  // Distinct per-worker sources and per-global-row/block raw learned
  // endpoints expose wrong peer, global row, bank or tile placement.
  for(unsigned i=0;i<16;i++)source[i]=i?0x12340000+worker*64+i:0x16005+(worker<<20);
  memory[0x7404c]=source[0]; // initial physical-source boot binding,BEFORE boot
  for(unsigned t=0;t<tiles.size();t++)for(unsigned row=0;row<16;row++){
   unsigned global=first+(t/(inputs/count))*16+row;
   std::vector<uint8_t> raw(PioneerQ38BlockBytes(type),0);
   uint16_t half=uint16_t(0x0800+(global%4)*0x80);
   if(type==8){raw[0]=uint8_t(half);raw[1]=uint8_t(half>>8);
    for(unsigned k=0;k<32;k++)raw[k+2]=uint8_t(int((t*17+global*19+k*37)%255)-127);
   }else{
    assert(type==14);
    for(unsigned i=0;i<128;i++)raw[i]=uint8_t((t*17+global*19+i*37)%256);
    for(unsigned i=128;i<192;i++)raw[i]=global&1?0x55:0xaa;
    for(unsigned i=192;i<208;i++)raw[i]=uint8_t((global&2)?-int(1+(t+i)%3):int(1+(t+i)%3));
    raw[208]=uint8_t(half);raw[209]=uint8_t(half>>8);
   }
   for(unsigned i=0;i<raw.size();i++)tiles[t][i*16+row]=raw[i];
  }
 }
};
struct DenseProjectionArithmetic:ProjectionMatrixMachine {
 std::vector<std::unique_ptr<DenseArithmeticPeer>>& actual;
 unsigned completed=0;
 DenseProjectionArithmetic(const char* image,const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38ProjectionJobs& jobs,
  unsigned layer,unsigned role,unsigned changed,std::vector<std::unique_ptr<DenseArithmeticPeer>>& pool)
  :ProjectionMatrixMachine(image,plan,jobs,layer,role,0,layer%7),actual(pool){
  assert(actual.size()==active);
  frame[6]+=changed;memory[0x74018]=frame[6];SetFrame();
  for(unsigned w=0;w<active;w++){
   auto& p=*actual[w];p.ClearTensorState();
   assert(p.pc==TT_Q38_PEER_WAIT_LOAD && !p.pending && p.inputs==k && p.type==kind && p.FfnPersistentPeer::groups==memory.at(0x74088+w*32));
   unsigned previous=p.memory.at(0x74144);peers[w].requested=peers[w].finished=previous;memory[0x7408c+w*32]=previous+1;
   p.base=memory.at(0x74080+w*32);memory[0x74094+w*32]=p.source[0];
   for(unsigned i=0;i<16;i++)memory[0x7d800+w*64+i*4]=p.source[i];
   for(unsigned i=p.inputReads;i<p.packets.size();i++){
    p.packets[i].address=p.base+(i-p.inputReads)*unsigned(p.tile.size());p.packets[i].route=memory.at(0x74090+w*32);
   }
  }
  // Changed RAW native endpoints, not precomputed Q/K/V/O products.
  if(changed){
   for(unsigned i=0;i<k;i++){
    uint16_t native=PioneerInt8Operand(int((i*43+changed*71)%255)-127);
    remoteInput[i/2]=(remoteInput[i/2]&~(0xffffU<<((i%2)*16)))|(uint32_t(native)<<((i%2)*16));
   }
   unsigned blocks=k>>(kind==8?5:8);
   for(unsigned i=0;i<blocks;i++)remoteScale[i]=Bits(kind==14 && (i&1)?-.0625f:.03125f);
  }
  assert(Shape());
 }
 uint32_t ExpectedOutput(unsigned w,unsigned row)const override{return actual.at(w)->buffers[4096+row];}
 void StepPeer(unsigned w){
  auto& p=*actual[w];auto& state=peers[w];
  for(unsigned i=0;i<10000;i++){
   if(p.pc==TT_Q38_PEER_FAULT_PARK || (p.memory.at(0x74144)==state.requested && p.pc==TT_Q38_PEER_WAIT_LOAD))break;
   p.StepBlock();
  }
  assert(p.pc!=TT_Q38_PEER_FAULT_PARK);
  state.finished=p.memory.at(0x74144);
 }
 void PeerPacket()override{
  bool write=command[7]==0x212;unsigned w=Worker(write?command[5]:command[2])-1;
  assert(w<active && pending && !unknown && !command[1] && !command[4] && !command[6] && !command[9]
   && (write?command[2]:command[5])==129 && (write || !command[7]));
  unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(local>=0x75000 && bytes && local+bytes<=0x75800 && !((local^remote)&31));
  auto& p=*actual[w];auto& state=peers[w];
  if(write && remote==0x74000){
   assert(bytes==64 && !doorbells && p.pc==TT_Q38_PEER_WAIT_LOAD);descriptors++;
   p.activation=false;p.Poison();p.PoisonTail();
   for(unsigned i=0;i<16;i++)state.descriptor[i]=p.memory[remote+i*4]=memory.at(local+i*4);
   assert(state.descriptor[10]==TT_Q38_FFN_PERSISTENT_MARKER && state.descriptor[11]==w+1
    && state.descriptor[12]==state.finished+1 && state.descriptor[13]==1 && state.descriptor[14]==1);
  }else if(write){
   assert(remote==0x74100 && bytes==4 && descriptors==active && !state.launched && !readbacks
    && memory.at(local)==state.finished+1);
   // The ROOT fetched these bytes through its emitted NoC commands. Only
   // endpoint copies populate the peer's input-read fabric.
   for(auto& pair:p.root)pair.second=memory.at(pair.first);
   for(unsigned i=0;i<k;i++)p.vector[i]=uint16_t(p.root.at(0xa0000+(i/2)*4)>>((i%2)*16));
   for(unsigned i=0;i<p.scales.size();i++)p.scales[i]=p.root.at(0xab000+i*4);
   p.memory[remote]=memory.at(local);state.requested=memory.at(local);state.launched=true;doorbells++;
  }else if(remote==0x74140){
   assert(bytes==64);if(state.launched)StepPeer(w);
   for(unsigned i=0;i<16;i++)memory[local+i*4]=p.memory.at(remote+i*4);
  }else if(remote==0xffb13140){assert(bytes==4);memory[local]=p.pc;}
  else if(remote==0xffb121b0){assert(bytes==4);memory[local]=0x46800;}
  else{
   assert(remote==0x83f00 && bytes==256+p.FfnPersistentPeer::groups*64 && AllFinished()
    && p.pc==TT_Q38_PEER_WAIT_LOAD && !p.pending);
   assert(p.requests==p.packets.size() && !p.dotProbeReads && p.CorrectRows());
   p.ClearTensorState();completed++;gathers++;
   for(unsigned i=0;i<64;i++)memory[local+i*4]=p.memory.at(remote+i*4);
   for(unsigned row=0;row<p.FfnPersistentPeer::groups*16;row++)memory[local+256+row*4]=p.buffers[4096+row];
  }
  Retire(bytes,write);
 }
 void VerifyActual(){
  Verify();assert(completed==active);
  for(unsigned w=0;w<active;w++){
   const auto& p=*actual[w];assert(p.memory.at(0x74144)==peers[w].requested && p.pc==TT_Q38_PEER_WAIT_LOAD);
   for(unsigned i=0;i<64;i++)assert(memory.at(0x76000+w*256+i*4)==p.memory.at(0x83f00+i*4));
   for(unsigned i=0;i<8;i++)assert(p.memory.at(0x83f20+i*4)==p.Count(i));
  }
 }
};
int main(int argc,char** argv){
 assert(argc==3);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 setvbuf(stdout,nullptr,_IOLBF,0);auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 unsigned matrices=0,rows=0,boots=0,peerJobs=0,minStack=0x70000;uint64_t instructions=0;
 for(unsigned role:{5U,6U,7U,10U}){
  unsigned layer=3;auto jobs=std::make_unique<PioneerQ38ProjectionJobs>();assert(jobs->Build(*plan,layer,role,123) && jobs->Waves()==1);
  std::vector<std::unique_ptr<DenseArithmeticPeer>> pool;
  for(unsigned w=0;w<jobs->Count();w++){
   const auto* part=jobs->Part(w);
   pool.emplace_back(new DenseArithmeticPeer(argv[2],jobs->Type(),jobs->Inputs(),part->groups,w+1,part->firstRow));
   assert(pool.back()->BootNew());boots++;
  }
  std::vector<uint32_t> previous;
  for(unsigned changed=0;changed<2;changed++){
   printf("START ACTUAL QDM1 dense role%u type%u K%u N%u workers%u changed%u;NOT_SILICON_TIME\n",
    role,jobs->Type(),jobs->Inputs(),jobs->Outputs(),jobs->Count(),changed);
   DenseProjectionArithmetic root(argv[1],*plan,*jobs,layer,role,changed,pool);
   assert(root.Run());root.VerifyActual();root.Inert();matrices++;rows+=jobs->Outputs();peerJobs+=root.completed;
   if(changed)assert(root.output!=previous);previous=root.output;
   minStack=std::min(minStack,root.minStack);
   for(auto& item:pool){
    auto& p=*item;instructions+=p.matrixInstructions;minStack=std::min(minStack,p.minStack);
    uint32_t physical[8];for(unsigned i=0;i<8;i++)physical[i]=p.Count(i);
    // Carry cumulative physical counts before clearing observer-only
    // packet bookkeeping; no hardware/private-sequence/Dst/Src reset.
    for(unsigned i=0;i<8;i++)p.countBase[i]=physical[i];p.NextPackets();
   }
   printf("PASS ACTUAL QDM1 role%u ALL%urows/%upeers changed%u exactFP32+independentFP64/ALL64actual proofwords retained;NOT_NATIVE_TIME\n",
    role,jobs->Outputs(),jobs->Count(),changed);
  }
  for(auto& p:pool)assert(p->Stop(p->memory.at(0x74144)+1));
 }
 printf("PASS OFFLINE ACTUAL dense projections matrices%u/rows%u/boots%u/peerJobs%u/matrixInstructions%llu/maxObservedStack%u;QG/K/V/Q6-O,TWO changed inputs,SAME retained peers/private sequences/cumulative counters/STOP,NO host numeric results in images;SYNTHETIC_ENDPOINTS_NOT_NATIVE_ORIGINAL_MODEL_FULL64_OR_SPEED\n",
  matrices,rows,boots,peerJobs,(unsigned long long)instructions,0x70000-minStack);
}
