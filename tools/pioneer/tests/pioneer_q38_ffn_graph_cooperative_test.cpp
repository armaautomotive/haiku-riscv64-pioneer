/* SPDX-License-Identifier: MIT */
// ACTUAL QFG1 root + ALL119 retained QFP2 RV32/FPU/Tensix interpreters.
// Fabric copies bytes ONLY;no synthetic completion,proofs or matrix products.
// Synthetic bounded nonzero weights,NOT real-model/native/timing evidence.
#define PIONEER_Q38_FFN_INPUT_MACHINE_ONLY
#define PIONEER_Q38_REF_DIAGNOSTIC
#define PIONEER_Q38_REFERENCE_ROUNDING_BOUND
#include "pioneer_q38_ffn_input_image_test.cpp"
#define PIONEER_Q38_DISPATCH_BASE_ALREADY_INCLUDED
#define PIONEER_Q38_FFN_GRAPH_MACHINE_ONLY
#include "pioneer_q38_ffn_graph_image_test.cpp"
struct GraphArithmeticPeer:FfnInputPeer {
 std::vector<uint32_t>& fabric;
 unsigned prepared=0,verified=0,operation=0,role=0,matrixJobs=0,producerJobs=0;
 uint64_t arithmetic=0;
 GraphArithmeticPeer(const char* path,unsigned worker,std::vector<uint32_t>& data)
  :FfnInputPeer(path,12,5120,1,worker),fabric(data) {assert(BootNew());}
 // Observe a ROOT-WRITTEN job. NEVER call Matrix/StageInput/StageActivation:
 // those unit-test helpers overwrite descriptors and could hide root bugs.
 void Prepare(const std::map<uint32_t,uint32_t>& rootMemory,unsigned currentRole,const FfnGraphFixture& fixture) {
  assert(pc==TT_Q38_PEER_WAIT_LOAD && !pending && prepared==verified);ClearTensorState();
  uint32_t counters[8];for(unsigned i=0;i<8;i++)counters[i]=Count(i);
  NextPackets();for(unsigned i=0;i<8;i++)countBase[i]=independentBefore[i]=counters[i];
  prepared=memory.at(0x74030);operation=memory.at(0x74034);role=currentRole;
  assert(prepared==verified+1 && memory.at(0x74100)==prepared);
  activation=operation==3 || operation==4;input=operation==4;
  activationPackets=commandReads=activatedWrites=nativeWrites=scaleWrites=0;
  native.fill(0xa5a5a5a5);scale.fill(0xa5a5a5a5);
  if(activation){for(unsigned i=0;i<64;i++)descriptor[i]=memory.at(0x74200+i*4);return;}
  if(operation==2)return;
  assert(operation==1 && role<3);
  const auto* plan=fixture.manifest.data()+PioneerQ38FfnGraphRoleOffset(role);
  const auto* part=plan+16+(logicalWorker-1)*8;
  const uint32_t canonical[]={0x31483851,plan[1],PioneerQ38PanelTileBytes(plan[1]),0,0,plan[3],part[0],part[2],part[4],1,
   TT_Q38_FFN_PERSISTENT_MARKER,logicalWorker,prepared,1,1,0};
  assert(logicalWorker<=plan[2]);for(unsigned i=0;i<16;i++)assert(memory.at(0x74000+i*4)==canonical[i]);
  type=memory.at(0x74004);inputs=memory.at(0x74014);groups=memory.at(0x7401c);base=memory.at(0x74018);
  assert((type==12 || type==14) && inputs==(role==2?17408U:5120U) && groups && groups<=16);
  count=256;vector.resize(inputs);scales.resize(inputs/256);tile.assign(PioneerQ38PanelTileBytes(type),0);
  // Different rows/blocks/workers/roles prevent a uniform output from hiding
  // stale jobs or bad row placement. All gates stay within the SiLU domain.
  tiles.assign(groups*(inputs/256),tile);
  for(unsigned t=0;t<tiles.size();t++)for(unsigned row=0;row<16;row++) {
   unsigned q=1+(t+row+logicalWorker+role)%3;
   uint16_t d=uint16_t(role==2?0x0100:role==0?0x0800:0x0c00); // exact FP16 powers:2^-16,-13,-12
   std::vector<uint8_t> raw(PioneerQ38BlockBytes(type),0);
   if(type==12){raw[0]=uint8_t(d);raw[1]=uint8_t(d>>8);
    for(unsigned i=0;i<4;i++)raw[4+i]=raw[12+i]=1;
    for(unsigned i=16;i<144;i++)raw[i]=uint8_t(q|(q<<4));
   }else{for(unsigned i=0;i<128;i++)raw[i]=uint8_t(q|(q<<4));
    for(unsigned i=128;i<192;i++)raw[i]=0xaa;for(unsigned i=192;i<208;i++)raw[i]=1;
    raw[208]=uint8_t(d);raw[209]=uint8_t(d>>8);
   }
   for(unsigned i=0;i<raw.size();i++)tiles[t][i*16+row]=raw[i];
  }
  root.clear();packets.clear();
  for(unsigned at=0;at<inputs*2;){unsigned bytes=std::min(16384U,inputs*2-at);
   packets.push_back({0xa0000+at,129,0xa0000+at,bytes,-1});
   for(unsigned i=0;i<bytes;i+=4)root[0xa0000+at+i]=rootMemory.at(0xa0000+at+i);at+=bytes;
  }
  unsigned padded=(unsigned(scales.size()*4)+63)&~63U;
  packets.push_back({0xab000,129,0x94000,padded,-1});
  for(unsigned at=0;at<padded;at+=4)root[0xab000+at]=rootMemory.at(0xab000+at);
  inputReads=unsigned(packets.size());
  for(unsigned t=0;t<tiles.size();t++)packets.push_back({base+t*unsigned(tile.size()),memory.at(0x74020),0x90000,unsigned(tile.size()),int(t)});
  for(unsigned k=0;k<inputs;k++)vector[k]=uint16_t(root.at(0xa0000+(k/2)*4)>>((k%2)*16));
  for(unsigned b=0;b<scales.size();b++)scales[b]=root.at(0xab000+b*4);
  // Poison only derived L1 caches and output canaries at a KNOWN clean WAIT.
  // Physical Dst/Src/CSR/registers/PC/private sequence are NEVER reset here.
  Poison();PoisonTail();for(unsigned row=0;row<4096;row++)buffers[4096+row]=0x7fc00000;
 }
 void SubmitActivation()override {
  // The base interpreter still audits every ACTUAL command and counter.
  // Populate endpoint bytes from the SAME GDDR workspace the root uses.
  const unsigned p=activationPackets;
  if(p==0)for(unsigned i=0;i<256;i++)gate[i]=fabric.at(descriptor[10]/4+i);
  if(!input && p==1)for(unsigned i=0;i<256;i++)up[i]=fabric.at(descriptor[11]/4+i);
  FfnInputPeer::SubmitActivation();
  if(p==(input?1U:2U))for(unsigned i=0;i<128;i++)fabric.at(descriptor[12]/4+i)=native[i];
  if(p==(input?2U:3U))for(unsigned i=0;i<16;i++)fabric.at(descriptor[13]/4+i)=scale[i];
 }
 void Advance() {
  if(prepared==verified)return;
  for(unsigned n=0;n<10000;n++) {
   if(pc==TT_Q38_PEER_FAULT_PARK)break;
   if(memory.at(0x74144)==prepared && (pc==TT_Q38_PEER_WAIT_LOAD || pc==TT_Q38_PEER_STOP))break;
   StepBlock();
  }
  if(pc==TT_Q38_PEER_FAULT_PARK){fprintf(stderr,"ACTUAL_PEER_FAULT worker%u op%u role%u PC%08x step%u\n",logicalWorker,operation,role,pc,steps);return;}
  if(memory.at(0x74144)!=prepared || (pc!=TT_Q38_PEER_WAIT_LOAD && pc!=TT_Q38_PEER_STOP))return;
  ClearTensorState();assert(!pending && memory.at(0x74140)==prepared && !memory.at(0x74148));
  if(operation==1){assert(requests==packets.size() && CorrectRows() && !dotProbeReads);matrixJobs++;arithmetic+=matrixInstructions;}
  if(operation==3){VerifyActivation();producerJobs++;}if(operation==4){VerifyInput();producerJobs++;}
  verified=prepared;
 }
};
struct GraphArithmeticRoot:GraphRootMachine {
 std::vector<std::unique_ptr<GraphArithmeticPeer>> actual;
 std::array<std::vector<uint32_t>,3> expected;
 unsigned assembled[3]={},finishedMatrices=0,finishedProducers=0;
 unsigned fault=0,observedPacket=0;
 GraphArithmeticRoot(const char* path,const char* peer,bool wide,unsigned bank,bool reverse,unsigned types,unsigned failure=0)
  :GraphRootMachine(path,wide,bank,(bank&1)!=0,reverse,types),fault(failure) {
  for(unsigned i=0;i<5120;i++)gddr[i]=Bits(1.0f+float((i*17+bank*11)%127)/256);
  for(unsigned role=0;role<3;role++)expected[role].assign(role==2?5120:17408,0x7fc00000);
  for(unsigned w=1;w<=119;w++)actual.emplace_back(new GraphArithmeticPeer(peer,w,gddr));
  if(fault==4)actual[0]->failPacket=1;
  if(fault==5)gddr[255]=0x7fc00001;
 }
 bool AfterInput(const GraphArithmeticPeer& p)const{return p.logicalWorker==1 && p.operation==4 && p.verified==p.prepared && p.verified;}
 uint32_t PhysicalRegister(GraphArithmeticPeer& p,unsigned at) {
  for(unsigned i=0;i<16;i++)if(at==0xffb20000+BlackholeDramSourceOffset(i)){
   if(fault==3 && i==11 && AfterInput(p)){observedPacket=packets;return p.source[i]^1;}return p.source[i];}
  for(unsigned i=0;i<8;i++)if(at==0xffb20000+p.CounterOffset(i)){
   if(fault==2 && !i && AfterInput(p)){observedPacket=packets;return p.Count(i)+1;}return p.Count(i);}
  assert(false && "unclassified/reserved actual peer CSR read");return 0;
 }
 void Record(unsigned w) {
  auto& p=*actual[w];auto& n=node[w];
  n.requested=p.memory.at(0x74140);n.sequence=p.memory.at(0x74144);n.stopped=p.pc==TT_Q38_PEER_STOP;
  if(n.sequence!=p.prepared || p.verified!=p.prepared || n.polls==p.verified)return;
  n.polls=p.verified;
  if(p.operation==1){const auto* m=fixture.manifest.data()+PioneerQ38FfnGraphRoleOffset(p.role);
   unsigned first=m[16+119*8+w];for(unsigned row=0;row<p.groups*16;row++)expected[p.role].at(first+row)=p.buffers[4096+row];
   assembled[p.role]+=p.groups*16;finishedMatrices++;
   if(!w || (w+1)%16==0 || w+1==m[2])printf("ACTUAL_GRAPH_MATRIX role%u worker%u/%u retainedSeq%u VMsteps%u;NOT_NATIVE_TIME\n",p.role,w+1,m[2],p.verified,p.steps);
  }else if(p.operation==3 || p.operation==4){finishedProducers++;
   if(!w || w+1==(p.operation==4?20U:68U))printf("ACTUAL_GRAPH_PRODUCER op%u worker%u retainedSeq%u VMsteps%u;NOT_NATIVE_TIME\n",p.operation,w+1,p.verified,p.steps);
  }
 }
 void Packet()override {
  const bool write=command[7]==0x212;unsigned route=write?command[5]:command[2];
  if(route==fixture.manifest[8]){GraphRootMachine::Packet();return;}
  assert(pending && !unknown && route!=1555 && !command[1] && !command[4] && !command[6] && !command[9]);
  assert((write?command[2]:command[5])==129 && (write || !command[7]));
  unsigned w=Worker(route)-1,local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(w<119 && local>=0x75000 && local+bytes<=0x75800 && !((local^remote)&31));auto& p=*actual[w];
  if(write){
   assert((remote==0x74000 && bytes==64) || (remote==0x74200 && bytes==256) || (remote==0x74100 && bytes==4));
   assert(p.pc==TT_Q38_PEER_WAIT_LOAD && p.prepared==p.verified);
   for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
   if(remote==0x74100){unsigned role=memory.count(0x7f914)?memory.at(0x7f914):0;p.Prepare(memory,role,fixture);assert(p.operation<=4);stages[p.operation]++;}
  }else if(remote==0x74140){assert(bytes==64);p.Advance();Record(w);
   for(unsigned i=0;i<16;i++)memory[local+i*4]=p.memory.at(remote+i*4);
  }else if(remote==0xffb13140){assert(bytes==4);memory[local]=p.pc;}
  else if(remote==0xffb121b0){assert(bytes==4);memory[local]=0x46800;}
  else if(remote==0x83f00){assert(p.prepared==p.verified && bytes==(p.operation==1?256+p.groups*64:256));
   for(unsigned i=0;i<64;i++)memory[local+i*4]=p.memory.at(remote+i*4);
   if(fault==1 && AfterInput(p)){memory[local+17*4]^=1;observedPacket=packets;}
   if(p.operation==1)for(unsigned i=0;i<p.groups*16;i++)memory[local+256+i*4]=p.buffers[4096+i];
  }else{for(unsigned i=0;i<bytes/4;i++)memory[local+i*4]=PhysicalRegister(p,remote+i*4);}
  counts[write?4:0]++;counts[write?5:1]+=(bytes+63)/64;counts[write?7:3]++;counts[2]++;responses++;pending=false;
 }
 void VerifyActual() {
  assert(!pending && !unknown && finishedProducers==88 && compactWrites==2);
  for(unsigned role=0;role<3;role++){
   assert(assembled[role]==expected[role].size());unsigned offset=role==0?30848:role==1?100480:274880;
   for(unsigned row=0;row<expected[role].size();row++)assert(gddr.at(offset/4+row)==expected[role][row] && isfinite(Float(expected[role][row])) && Float(expected[role][row])>0);
  }
  for(unsigned w=0;w<119;w++){const auto& p=*actual[w];assert(p.prepared==p.verified && p.pc==TT_Q38_PEER_WAIT_LOAD && !p.dotProbeReads);
   assert(p.verified==PioneerQ38FfnGraphNextPeer(fixture.manifest.data(),w));}
 }
 void RelaunchRoot(){for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++)memory[TT_Q38_FFN_GRAPH_MANIFEST+i*4]=fixture.manifest[i];
  pc=0x6000;regs[0]=0;captures=0;startPackets=packets;
 }
 void StageNext(){VerifyActual();auto& d=fixture.manifest;d[6]=d[13];d[13]=d[6]+5;d[40]++;d[42]++;
  for(unsigned w=0;w<119;w++)d[64+w]=actual[w]->verified;
  assert(PioneerQ38FfnGraphManifest(d.data(),fixture.source.data()));
  // A second independently staged TEST input/token/layer,not a CPU FFN
  // result or native initial-producer receipt. Every private peer persists.
  for(unsigned i=0;i<5120;i++)gddr[i]=Bits(.5f+float((i*31+17)%127)/512);
  for(unsigned role=0;role<3;role++){assembled[role]=0;std::fill(expected[role].begin(),expected[role].end(),0x7fc00000);}
  finishedMatrices=finishedProducers=compactWrites=0;RelaunchRoot();
 }
 void StageStop(){VerifyActual();auto& d=fixture.manifest;d[5]=2;d[6]=d[13];d[13]=d[6]+1;d[14]=0;
  for(unsigned w=0;w<119;w++)d[64+w]=actual[w]->verified;
  assert(PioneerQ38FfnGraphManifest(d.data(),fixture.source.data()));
  RelaunchRoot();
 }
};
int main(int argc,char** argv) {
 assert((argc==7 || argc==8) && (!strcmp(argv[3],"all119") || !strcmp(argv[3],"narrow")));setvbuf(stdout,nullptr,_IOLBF,0);
 unsigned bank=unsigned(strtoul(argv[4],nullptr,10)),types=unsigned(strtoul(argv[5],nullptr,10)),reverse=unsigned(strtoul(argv[6],nullptr,10));
 unsigned fault=argc==8?unsigned(strtoul(argv[7],nullptr,10)):0;
 assert(bank<7 && types<8 && reverse<2 && fault<=5);bool wide=!strcmp(argv[3],"all119");
 printf("START ACTUAL complete7stage FFN root+ALL119 retained peers workforce%s bank%u types%u reverse%u;SYNTHETIC_NONZERO_WEIGHTS/NO_NATIVE_SPEED_CLAIM\n",argv[3],bank,types,reverse);
 GraphArithmeticRoot root(argv[1],argv[2],wide,bank,reverse!=0,types,fault);
 if(fault){assert(!root.RunGraph() && root.unknown && !root.finishedMatrices);
  if(fault<=3)assert(root.observedPacket && root.packets==root.observedPacket);
  else{assert(root.actual[0]->pc==TT_Q38_PEER_FAULT_PARK);root.actual[0]->Inert();}
  root.Inert();printf("PASS ACTUAL wholeFFN fault%u:root parks/no following packet/no matrix/no STOP/reset/retry;simulated endpoints ONLY\n",fault);return 0;
 }
 if(!root.RunGraph()){fprintf(stderr,"ACTUAL_GRAPH_FAILURE rootPC%08x stage%08x detail%u packets%u matrices%u producers%u\n",root.pc,root.l1[root.Index(0x17ffc0)],root.l1[root.Index(0x17ffc4)],root.packets,root.finishedMatrices,root.finishedProducers);return 1;}
 root.VerifyActual();auto first=root.expected[2];root.StageNext();assert(root.RunGraph());root.VerifyActual();assert(first!=root.expected[2]);
 uint64_t steps=0,math=0;unsigned minStack=root.minStack;
 for(const auto& p:root.actual){steps+=p->steps;math+=p->arithmetic;minStack=std::min(minStack,p->minStack);}
 root.StageStop();assert(root.RunGraph());for(const auto& p:root.actual)assert(p->pc==TT_Q38_PEER_STOP && p->prepared==p->verified);
 printf("PASS OFFLINE ACTUAL ROOT+ALL119 retained peers TWOchangedInputTokenLayerFFNs 5120->17408->5120/79872nonzero matrix rows/176actual producers/ALL119STOP;EXACT_FP32+INDEPENDENT_FP64+NATIVE_BYTE_OBSERVERS/noCPUproducts_in_program/noPeerReset/rootVM%u peerVM%llu TensixInstructions%llu maxStack%u;NO_REAL_MODEL_NATIVE_OWNER_OR_SPEED_QUALIFICATION\n",root.steps,(unsigned long long)steps,(unsigned long long)math,0x70000-minStack);
}
