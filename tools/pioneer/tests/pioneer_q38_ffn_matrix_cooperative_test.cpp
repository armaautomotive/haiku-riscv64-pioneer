/* SPDX-License-Identifier: MIT */
// Entire projection:ACTUAL new root AND ALL actual qualified peer images.
// Only NoC/GDDR/reset/cycle endpoints simulated. Weights are synthetic test
// tiles,not the real model. This is NOT silicon timing or native ownership.
#define PIONEER_Q38_TEST_UNSELECTED_OUTPUT_ROOT
#define PIONEER_Q38_TEST_PRIVATE_FFN_ROOT
#define PIONEER_Q38_COOPERATIVE_MOCK_ONLY
#include "pioneer_qwen38_dispatch_cooperative_test.cpp"
#define PIONEER_Q38_FFN_MATRIX_DISPATCH_INCLUDED
#define PIONEER_Q38_FFN_MATRIX_MACHINE_ONLY
#include "pioneer_q38_ffn_matrix_image_test.cpp"
struct FfnMatrixCooperative:FfnMatrixMachine {
 std::vector<std::unique_ptr<CooperativePeer>> actual;
 std::vector<bool> reported;
 FfnMatrixCooperative(const char* root,const char* peer,unsigned role,unsigned type)
  :FfnMatrixMachine(root,role,0,false,0,type,role==2?20:68),reported(active,false) {
  for(unsigned w=0;w<active;w++) {
   unsigned groups=memory.at(0x74088+w*32);
   actual.emplace_back(new CooperativePeer(peer,type,k,groups,w+1));auto& p=*actual.back();assert(p.Boot());
   p.base=memory.at(0x74080+w*32);
   for(unsigned i=p.inputReads;i<p.packets.size();i++) {
    p.packets[i].address=p.base+(i-p.inputReads)*unsigned(p.tile.size());p.packets[i].route=memory.at(0x74090+w*32);
   }
   p.PoisonTail();for(unsigned i=0;i<k;i++)p.vector[i]=uint16_t(remoteInputs[i/2]>>((i%2)*16));
   for(unsigned i=0;i<p.scales.size();i++)p.scales[i]=remoteScales[i];
  }
 }
 uint32_t ExpectedOutput(unsigned w,unsigned row)const override{return actual.at(w)->buffers[4096+row];}
 void StepPeer(unsigned w) {
  auto& p=*actual[w];
  for(unsigned step=0;step<10000;step++) {
   if(p.pc==TT_Q38_PEER_FAULT_PARK || (p.memory.at(0x74144)==peers[w].requested && (p.pc==0x7204c || p.pc==0x72060)))break;
   p.StepBlock();
  }
  peers[w].requested=p.memory.at(0x74140);peers[w].finished=p.memory.at(0x74144);
  if(peers[w].finished==peers[w].requested && peers[w].finished && !reported[w]) {
   reported[w]=true;if(!w || (w+1)%8==0 || w+1==active)printf("ACTUAL_FFN_PEER_COMPLETE worker%u/%u VMsteps%u;NOT_NATIVE_TIME\n",w+1,active,p.steps);
  }
 }
 void Packet()override {
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2];
  if(route==frame[8]){FfnMatrixMachine::Packet();return;}
  assert(pending && !unknown && route!=1555);unsigned worker=Worker(route),w=worker-1;
  assert(w<actual.size() && !command[1] && !command[4] && !command[6] && !command[9]);
  assert((write?command[2]:command[5])==129 && (write || !command[7]));
  unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(local>=0x75000 && bytes && local+bytes<=0x75800 && !((local^remote)&31));
  auto& p=*actual[w];auto& state=peers[w];
  if(write && remote==0x74000) {
   assert(bytes==64 && !doorbells && p.pc==0x7204c);descriptors++;
   for(unsigned i=0;i<16;i++)state.descriptor[i]=p.memory[remote+i*4]=memory.at(local+i*4);
   assert(state.descriptor[11]==worker && state.descriptor[12]==state.finished+1 && state.descriptor[13]==1 && state.descriptor[14]==1);
  }else if(write) {
   assert(remote==0x74100 && bytes==4 && descriptors==active && !readbacks && !state.launched);
   assert(memory.at(local)==state.descriptor[12]);p.memory[remote]=memory.at(local);
   state.requested=memory.at(local);state.launched=true;state.operation=1;doorbells++;
   // Every actual peer NoC input read now sees ONLY the root's freshly
   // fetched resident native data,not the poisoned pre-fetch L1 dictionary.
   for(auto& value:p.root)value.second=memory.at(value.first);
  }else if(remote==0x74140) {
   assert(bytes==64);if(state.launched)StepPeer(w);
   for(unsigned i=0;i<16;i++)memory[local+i*4]=p.memory.at(remote+i*4);
  }else if(remote==0xffb13140){assert(bytes==4);memory[local]=p.pc;}
  else if(remote==0xffb121b0){assert(bytes==4);memory[local]=0x46800;}
  else {
   assert(remote==0x83f00 && bytes==256+p.groups*64 && AllFinished());gathers++;
   for(const auto& peer:actual)assert((peer->pc==0x7204c || peer->pc==0x72050) && !peer->pending);
   for(unsigned i=0;i<64;i++)memory[local+i*4]=p.memory.at(remote+i*4);
   for(unsigned row=0;row<p.groups*16;row++)memory[local+256+row*4]=p.buffers[4096+row];
  }
  counts[write?4:0]++;counts[write?5:1]+=(bytes+63)/64;counts[write?7:3]++;counts[2]++;responses++;pending=false;
 }
};
int main(int argc,char** argv) {
 assert(argc==5 && strlen(argv[3])==1 && argv[3][0]>='0' && argv[3][0]<='2' && (!strcmp(argv[4],"12") || !strcmp(argv[4],"14")));
 setvbuf(stdout,nullptr,_IOLBF,0);unsigned role=unsigned(argv[3][0]-'0'),type=!strcmp(argv[4],"12")?12:14;
 printf("START FULL ACTUAL FFN_PROJECTION role%u type%u;ALL%uPEERS/fullK%u/fullrows%u/syntheticweighttiles/NATIVE_NOT_TOUCHED\n",role,type,role==2?20:68,role==2?17408:5120,role==2?5120:17408);
 FfnMatrixCooperative m(argv[1],argv[2],role,type);assert(m.RunFfn());m.VerifyFfn();uint64_t peerSteps=0;
 for(const auto& p:m.actual){assert(p->CorrectRows() && !p->dotProbeReads);peerSteps+=p->steps;}
 printf("PASS OFFLINE FULL ACTUAL ROOT+ALL%uPEERS role%u type%u K%u rows%u;GDDRnativeinput->actualunpack/matrix/affine->GDDRoutput/ALL_FP32_AND_FP64_OBSERVERS/noCPUproducts_or_hostPCI_in_program/rootVM%u/peerVM%llu;NO_REAL_MODEL_NATIVE_LIFETIME_OR_SPEED_QUALIFICATION\n",m.active,role,type,m.k,m.frame[11],m.steps,(unsigned long long)peerSteps);
}
