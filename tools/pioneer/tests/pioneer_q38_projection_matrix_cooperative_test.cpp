/* SPDX-License-Identifier: MIT */
// ACTUAL op5 pool -> ACTUAL QPG1 gather -> ACTUAL QPM1 ROOT+ALL active
// QFP3 peers -> complete projection GDDR output. Fabric COPIES BYTES ONLY.
// Synthetic bounded test weights;NOT native owners/real model or timing.
#define PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_PROJECTION_GATHER_MACHINE_ONLY
#include "pioneer_q38_projection_gather_image_test.cpp"
#define PIONEER_Q38_DISPATCH_BASE_ALREADY_INCLUDED
#define PIONEER_Q38_PROJECTION_MATRIX_MACHINE_ONLY
#include "pioneer_q38_projection_matrix_image_test.cpp"
struct ProjectionMatrixCooperative:ProjectionMatrixMachine {
 std::vector<std::unique_ptr<ProjectionPeer>>& actual;
 ProjectionMatrixCooperative(const char* image,const PioneerQ38CanonicalModelPlan& plan,const PioneerQ38ProjectionJobs& jobs,
  unsigned layer,unsigned role,std::vector<std::unique_ptr<ProjectionPeer>>& pool)
  :ProjectionMatrixMachine(image,plan,jobs,layer,role,0),actual(pool){
  assert(actual.size()>=active);
  for(unsigned w=0;w<active;w++){
   auto& p=*actual[w];assert(p.pc==TT_Q38_PEER_WAIT_LOAD && !p.pending && p.inputs==k && p.type==kind && p.groups==memory.at(0x74088+w*32));
   unsigned previous=p.memory.at(0x74144);peers[w].requested=peers[w].finished=previous;memory[0x7408c+w*32]=previous+1;
   p.base=memory.at(0x74080+w*32);
   for(unsigned i=p.inputReads;i<p.packets.size();i++){
    p.packets[i].address=p.base+(i-p.inputReads)*unsigned(p.tile.size());p.packets[i].route=memory.at(0x74090+w*32);
   }
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
  assert(p.pc!=TT_Q38_PEER_FAULT_PARK);state.requested=p.memory.at(0x74140);state.finished=p.memory.at(0x74144);
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
   p.projection=p.activation=p.input=false;p.Poison();p.PoisonTail();
   for(unsigned i=0;i<16;i++)state.descriptor[i]=p.memory[remote+i*4]=memory.at(local+i*4);
   assert(state.descriptor[10]==TT_Q38_FFN_PERSISTENT_MARKER && state.descriptor[11]==w+1
    && state.descriptor[12]==state.finished+1 && state.descriptor[13]==1 && state.descriptor[14]==1);
  }else if(write){
   assert(remote==0x74100 && bytes==4 && descriptors==active && !state.launched && !readbacks
    && memory.at(local)==state.finished+1);
   // Fresh actual ROOT bytes ONLY,not CPU-reconstructed quantization.
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
   assert(remote==0x83f00 && bytes==256+p.groups*64 && AllFinished());gathers++;
   for(unsigned i=0;i<active;i++)assert(actual[i]->pc==TT_Q38_PEER_WAIT_LOAD && !actual[i]->pending);
   for(unsigned i=0;i<64;i++)memory[local+i*4]=p.memory.at(remote+i*4);
   for(unsigned row=0;row<p.groups*16;row++)memory[local+256+row*4]=p.buffers[4096+row];
  }
  Retire(bytes,write);
 }
};
int main(int argc,char** argv){
 assert(argc==4);(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;(void)&FeedMatrix;
 setvbuf(stdout,nullptr,_IOLBF,0);auto plan=std::make_unique<PioneerQ38CanonicalModelPlan>();assert(plan->Build());
 unsigned producers=0,matrices=0,rows=0,boots=0;uint64_t instructions=0;
 // FULL48-row recurrent projection and FULL1024-row attention key. Both
 // have one16-row job/worker;larger/multi-wave geometry is a separate root
 // instruction test. Do NOT report these as all498 real arithmetic runs.
 for(unsigned role:{10U,6U}){
  unsigned layer=role==6?3:0;auto jobs=std::make_unique<PioneerQ38ProjectionJobs>();assert(jobs->Build(*plan,layer,role,123));
  assert(jobs->Type()==8 && jobs->Inputs()==5120 && jobs->Waves()==1);unsigned active=jobs->WaveWorkers(0),poolSize=std::max(active,20U);
  std::vector<std::unique_ptr<ProjectionPeer>> pool;
  for(unsigned w=0;w<poolSize;w++){
   pool.emplace_back(new ProjectionPeer(argv[2],8,5120,w+1));auto& peer=*pool.back();
   // Different worker/row/block weight bytes prevent identical repeated
   // tiles from hiding a misplaced global row. Only endpoint test weights
   // are initialized here;device program/Dst/Src state is never patched.
   for(unsigned t=0;t<peer.tiles.size();t++){
    auto& tile=peer.tiles[t];std::fill(tile.begin(),tile.end(),0);
    for(unsigned row=0;row<16;row++){
     uint16_t half=uint16_t(0x0800+((w+row)%4)*0x100);tile[row]=uint8_t(half);tile[16+row]=uint8_t(half>>8);
     for(unsigned lane=0;lane<32;lane++)tile[(lane+2)*16+row]=uint8_t(int((t*17+w*13+row*19+lane*37)%255)-127);
    }
   }
   assert(peer.BootNew());boots++;
  }
  std::vector<uint32_t> first(jobs->Outputs(),0);
  for(unsigned changed=0;changed<2;changed++){
   ProjectionGatherMachine gather(argv[3],layer,role,0,false,17+changed);
   gather.descriptor[4]=0x04000000+plan->Workspace(0)->offset;gather.SetDescriptor();
   for(unsigned chunk=0;chunk<20;chunk++){
    auto& p=*pool[chunk];unsigned sequence=p.memory.at(0x74144)+1;p.Stage(sequence,layer,role,17+changed);
    p.descriptor[4]=gather.descriptor[4];for(unsigned i=0;i<64;i++)p.memory[TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4]=p.descriptor[i];
    for(unsigned i=0;i<256;i++)p.gate[i]=Bits(float(int((chunk*17+i*37+changed*71)%255)-127)*.125f);
    assert(p.Finish(sequence));p.Verify();producers++;
    assert(p.descriptor[3]==gather.descriptor[3] && p.descriptor[4]==gather.descriptor[4]
     && p.descriptor[12]==gather.descriptor[10]+chunk*512 && p.descriptor[13]==gather.descriptor[11]+chunk*128);
    memcpy(gather.native.data()+chunk*128,p.native.data(),512);gather.lines[chunk]=p.line;
   }
   assert(gather.Run());gather.Verify();ProjectionMatrixCooperative root(argv[1],*plan,*jobs,layer,role,pool);
   root.frame[6]=17+changed;root.memory[0x74018]=root.frame[6];root.SetFrame();assert(root.Shape());
   assert(root.frame[3]==gather.descriptor[3] && root.frame[4]==gather.descriptor[4]
    && gather.descriptor[10]==TT_Q38_PROJECTION_NATIVE_OFFSET && gather.descriptor[12]==TT_Q38_PROJECTION_COMPACT_OFFSET);
   for(unsigned i=0;i<root.remoteInput.size();i++)root.remoteInput[i]=gather.memory.at(0xa0000+i*4);
   for(unsigned i=0;i<root.remoteScale.size();i++)root.remoteScale[i]=gather.memory.at(0xab000+i*4);
   assert(root.Run());root.Verify();matrices++;rows+=jobs->Outputs();bool different=false;
   for(unsigned row=0;row<jobs->Outputs();row++){
    assert(root.cover[row]==1);if(!changed)first[row]=root.output[row];else different|=first[row]!=root.output[row];
   }
   if(changed)assert(different);
   for(unsigned w=0;w<active;w++){
    auto& p=*pool[w];assert(p.CorrectRows() && !p.dotProbeReads && p.requests==p.packets.size());instructions+=p.matrixInstructions;
    uint32_t physical[8];for(unsigned i=0;i<8;i++)physical[i]=p.Count(i);
    for(unsigned i=0;i<8;i++)p.countBase[i]=physical[i];p.NextPackets(); // observer job counters ONLY,not hardware/CSR reset
   }
   printf("PASS ACTUAL op5->QPG1->QPM1+ALL%u retained QFP3 peers FULL%urows changed%u/FP32+FP64 observers;GDDR intermediates/FABRIC_BYTES_ONLY/NO reset,NOT_NATIVE_TIME\n",active,jobs->Outputs(),changed);
  }
  for(auto& p:pool)assert(p->Stop(p->memory.at(0x74144)+1));
 }
 printf("PASS OFFLINE complete projection arithmetic:producerJobs%u/matrices%u/ALLoutputRows%u/boots%u/matrixInstructions%llu;TWO changed inputs retained pool/STOP,zero host PCI in images,test-only references;NO_NATIVE_OWNER_FULL498MODEL_OR_SPEED_GRANT\n",producers,matrices,rows,boots,(unsigned long long)instructions);
}
