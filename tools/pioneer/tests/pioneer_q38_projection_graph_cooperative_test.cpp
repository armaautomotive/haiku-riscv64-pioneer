/* SPDX-License-Identifier: MIT */
// ACTUAL parent+ALL119 retained QFP3 RV32/FPU/Tensix machines. Endpoints
// copy bytes ONLY;peer status/proofs/products come from actual instructions.
// Test weights/input/transport/reset endpoints are simulated,NOT native.
#ifndef PIONEER_Q38_GDN_MUTABLE_STATE
#define PIONEER_Q38_GDN_MUTABLE_STATE
#endif
#ifdef PIONEER_Q38_TEST_PROJECTION_FOOTPRINT
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionGraphFootprint.h"
#endif
#define PIONEER_Q38_PROJECTION_MACHINE_ONLY
#include "pioneer_q38_projection_persistent_image_test.cpp"
#define PIONEER_Q38_DISPATCH_BASE_ALREADY_INCLUDED
#define PIONEER_Q38_PROJECTION_PARENT_MACHINE_ONLY
#include "pioneer_q38_projection_graph_image_test.cpp"
struct ParentArithmeticPeer:ProjectionPeer {
 std::vector<uint32_t>& fabric;
#ifdef PIONEER_Q38_TEST_PROJECTION_FOOTPRINT
 uint64_t observedL1Stores=0;
 void Store(uint32_t a,uint32_t value)override{
  if(a<0xff000000){
   assert(PioneerQ38ProjectionGraphSnapshotContains(logicalWorker,a)
    && !PioneerQ38ProjectionGraphProgramImmutable(logicalWorker,a));observedL1Stores++;
  }
  ProjectionPeer::Store(a,value);
 }
#endif
 ParentArithmeticPeer(const char* image,unsigned worker,std::vector<uint32_t>& data,bool boot=true):ProjectionPeer(image,12,5120,worker),fabric(data){if(boot)assert(BootNew());}
 virtual ~ParentArithmeticPeer()=default;
 virtual bool ExternalLearnedBytes()const{return false;}
 virtual bool ReadLearnedBytes(unsigned,uint32_t,uint8_t*,unsigned){return false;}
 void SubmitActivation()override{
  assert(projection);unsigned packet=activationPackets;
  if(!packet)for(unsigned i=0;i<256;i++)gate[i]=fabric.at(descriptor[10]/4+i);
  ProjectionPeer::SubmitActivation();if(pending)return;
  if(packet==1)for(unsigned i=0;i<128;i++)fabric.at(descriptor[12]/4+i)=native[i];
  if(packet==2)for(unsigned i=0;i<32;i++)fabric.at(descriptor[13]/4+i)=line[i];
 }
 void Prepare(const std::map<uint32_t,uint32_t>& rootMemory){
  assert(pc==TT_Q38_PEER_WAIT_LOAD && !pending);ClearTensorState();
  uint32_t physical[8];for(unsigned i=0;i<8;i++)physical[i]=Count(i);
  NextPackets();for(unsigned i=0;i<8;i++)countBase[i]=independentBefore[i]=physical[i];
  unsigned operation=memory.at(0x74034);activation=projection=operation==5;input=false;
  activationPackets=commandReads=activatedWrites=nativeWrites=scaleWrites=lineWrites=0;observed=false;
  if(operation==5){for(unsigned i=0;i<64;i++)descriptor[i]=memory.at(TT_Q38_FFN_RESIDENT_DESCRIPTOR+i*4);return;}
  if(operation==2)return;
  assert(operation==1 && memory.at(0x74038)==1);
  // Observer geometry/endpoint fixture only. NEVER overwrite a ROOT job,
  // source/CSR/PC/private nonce/Dst/Src/register or call unit Stage/Matrix.
  type=memory.at(0x74004);inputs=memory.at(0x74014);groups=memory.at(0x7401c);base=memory.at(0x74018);
  assert((type==8 || type==12 || type==14) && (inputs==5120 || inputs==6144 || inputs==17408) && groups && groups<=16);
  count=type==8?32:256;vector.resize(inputs);scales.resize(inputs/count);tile.assign(PioneerQ38PanelTileBytes(type),0);
  tiles.assign(groups*(inputs/count),tile);unsigned first=rootMemory.at(TT_Q38_PROJECTION_MATRIX_ROWS+(logicalWorker-1)*4);
  if(ExternalLearnedBytes()){
   for(unsigned t=0;t<tiles.size();t++)assert(ReadLearnedBytes(memory.at(0x74020),base+t*unsigned(tile.size()),tiles[t].data(),unsigned(tile.size())));
  }else for(unsigned t=0;t<tiles.size();t++)for(unsigned row=0;row<16;row++){
   unsigned global=first+(t/(inputs/count))*16+row+((base>>6)%251)*16;std::vector<uint8_t> raw(PioneerQ38BlockBytes(type),0);
   uint16_t half=uint16_t(0x0800+(global%4)*0x80);
   if(type==8){raw[0]=uint8_t(half);raw[1]=uint8_t(half>>8);
    for(unsigned k=0;k<32;k++)raw[k+2]=uint8_t(int((t*17+global*19+k*37)%255)-127);
   }else if(type==12){
    raw[0]=uint8_t(half);raw[1]=uint8_t(half>>8);uint16_t minimum=global%3?0x0400:0;
    raw[2]=uint8_t(minimum);raw[3]=uint8_t(minimum>>8);
    for(unsigned i=4;i<16;i++)raw[i]=uint8_t(1+(t+global+i)%3);
    for(unsigned i=16;i<144;i++)raw[i]=uint8_t((t*17+global*19+i*37)%256);
   }else{
    for(unsigned i=0;i<128;i++)raw[i]=uint8_t((t*17+global*19+i*37)%256);
    for(unsigned i=128;i<192;i++)raw[i]=global&1?0x55:0xaa;
    for(unsigned i=192;i<208;i++)raw[i]=uint8_t((global&2)?-int(1+(t+i)%3):int(1+(t+i)%3));
    raw[208]=uint8_t(half);raw[209]=uint8_t(half>>8);
   }
   for(unsigned i=0;i<raw.size();i++)tiles[t][i*16+row]=raw[i];
  }
  root.clear();packets.clear();
  for(unsigned at=0;at<inputs*2;){unsigned bytes=std::min(16384U,inputs*2-at);
   packets.push_back({0xa0000+at,129,0xa0000+at,bytes,-1});
   for(unsigned i=0;i<bytes;i+=4)root[0xa0000+at+i]=rootMemory.at(0xa0000+at+i);at+=bytes;
  }
  unsigned padded=(unsigned(scales.size()*4)+63)&~63U;packets.push_back({0xab000,129,0x94000,padded,-1});
  for(unsigned at=0;at<padded;at+=4)root[0xab000+at]=rootMemory.at(0xab000+at);
  inputReads=unsigned(packets.size());
  for(unsigned t=0;t<tiles.size();t++)packets.push_back({base+t*unsigned(tile.size()),memory.at(0x74020),0x90000,unsigned(tile.size()),int(t)});
  for(unsigned k=0;k<inputs;k++)vector[k]=uint16_t(root.at(0xa0000+(k/2)*4)>>((k%2)*16));
  for(unsigned b=0;b<scales.size();b++)scales[b]=root.at(0xab000+b*4);
  Poison();PoisonTail();for(unsigned row=0;row<4096;row++)buffers[4096+row]=0x7fc00000;
 }
};
struct ArithmeticParent:ProjectionParentMachine {
#ifdef PIONEER_Q38_TEST_PROJECTION_FOOTPRINT
 uint64_t observedL1Stores=0;
 void Store(uint32_t a,uint32_t value)override{
  if(a<0xff000000){
   assert(PioneerQ38ProjectionGraphSnapshotContains(0,a)
    && !PioneerQ38ProjectionGraphProgramImmutable(0,a));observedL1Stores++;
  }
  ProjectionParentMachine::Store(a,value);
 }
#endif
 std::array<std::unique_ptr<ParentArithmeticPeer>,119> actual;
 std::vector<uint32_t> expected;
 unsigned boots=0,runs=0,completedMatrices=0,completedProducers=0,completedStops=0;
 uint64_t matrixInstructions=0;
 ArithmeticParent(const char* image,const char* peer,bool initialize=true,unsigned initialLayer=0,unsigned initialBank=0):ProjectionParentMachine(image,initialLayer,10,initialBank){
  if(initialize){
   for(unsigned w=0;w<119;w++){actual[w]=std::make_unique<ParentArithmeticPeer>(peer,w+1,gddr);boots++;}
   Retarget(initialLayer,10,false);
  }
 }
 void Retarget(unsigned layer,unsigned role,bool stop){
  // Known clean held/terminal ROOT ONLY;peers/Dst/Src/CSR/private sequence
  // NEVER reset. The native owner/mappings needed to do this are NOT
  // supplied by this test fixture or a caller ready/producer flag.
  assert(!pending && !unknown);
  fixture.layer=layer;fixture.role=role;fixture.stop=stop;fixture.input=stop?0:UINT64_C(0x10000002a)+runs;
  fixture.token=123+runs;fixture.phase=runs?memory.at(TT_Q38_PROJECTION_GRAPH_STATS+54*4):0;
  fixture.jobs=std::make_unique<PioneerQ38ProjectionJobs>();assert(fixture.jobs->Build(*fixture.canonical,layer,role,fixture.token));
  fixture.graph=std::make_unique<PioneerQ38ProjectionGraphPlan>();
  for(unsigned w=0;w<119;w++){
   auto& p=*actual[w];assert(p.pc==TT_Q38_PEER_WAIT_LOAD && !p.pending);fixture.sequence[w]=p.memory.at(0x74144);
   for(unsigned i=0;i<16;i++)fixture.sources[w][i]=p.source[i];
   node[w].sequence=node[w].requested=fixture.sequence[w];node[w].op=0;node[w].stopped=false;node[w].verified=true;
   for(unsigned i=0;i<8;i++)node[w].count[i]=p.Count(i);
  }
  assert(fixture.Build());kind=fixture.jobs->Type();k=fixture.jobs->Inputs();Stage();
  outputCover.assign(fixture.jobs->Outputs(),0);expected.assign(fixture.jobs->Outputs(),0x7fc00001);
  for(unsigned i=0;i<k;i++)gddr[i]=Bits(float(int((i*37+runs*71)%255)-127)*.125f);
  producerJobs=matrixJobs=stopJobs=lineReads=operandReads=scaleWrites=scaleReads=outputWrites=0;
  pc=0x6000;regs[0]=0; // ROOT frontend re-entry ONLY;no peer/hardware reset
 }
 virtual void InputFinished(ParentArithmeticPeer& p){p.Verify();}
 void Advance(unsigned w)override{
  auto& p=*actual[w];auto& n=node[w];if(n.verified)return;
  for(unsigned i=0;i<10000;i++){
   if(p.pc==TT_Q38_PEER_FAULT_PARK || (p.memory.at(0x74144)==n.requested && (p.pc==TT_Q38_PEER_WAIT_LOAD || p.pc==TT_Q38_PEER_STOP)))break;
   p.StepBlock();
  }
  if(p.pc==TT_Q38_PEER_FAULT_PARK){fprintf(stderr,"ACTUAL_PARENT_PEER_FAULT w%u op%u PC%08x steps%u\n",w+1,n.op,p.pc,p.steps);assert(false);}
  n.requested=p.memory.at(0x74140);n.sequence=p.memory.at(0x74144);for(unsigned i=0;i<8;i++)n.count[i]=p.Count(i);
  if(n.sequence!=n.requested || (p.pc!=TT_Q38_PEER_WAIT_LOAD && p.pc!=TT_Q38_PEER_STOP))return;
  p.ClearTensorState();assert(!p.pending && !p.memory.at(0x74148));n.verified=true;n.stopped=p.pc==TT_Q38_PEER_STOP;
  if(n.op==1){
   assert(p.CorrectRows() && !p.dotProbeReads && p.requests==p.packets.size());completedMatrices++;matrixJobs++;matrixInstructions+=p.matrixInstructions;
   unsigned first=memory.at(TT_Q38_PROJECTION_MATRIX_ROWS+w*4);for(unsigned row=0;row<p.groups*16;row++)expected.at(first+row)=p.buffers[4096+row];
  }else if(n.op==5 || n.op==6){InputFinished(p);completedProducers++;producerJobs++;}else{assert(n.op==2 && n.stopped);completedStops++;stopJobs++;}
 }
 uint32_t PhysicalRegister(unsigned w,unsigned a)override{
  auto& p=*actual[w];if(a==0xffb13140)return p.pc;if(a==0xffb121b0)return 0x46800;
  for(unsigned i=0;i<16;i++)if(a==0xffb20000+BlackholeDramSourceOffset(i))return p.source[i];
  for(unsigned i=0;i<8;i++)if(a==0xffb20000+p.CounterOffset(i))return p.Count(i);
  assert(false && "reserved physical register");return 0;
 }
 uint32_t RowOutput(unsigned w,unsigned row)const override{return actual[w]->buffers[4096+row];}
 void PeerPacket()override{
  bool write=command[7]==0x212;unsigned w=Worker(write?command[5]:command[2])-1;auto& n=node[w];auto& p=*actual[w];
  unsigned local=write?command[0]:command[3],remote=write?command[3]:command[0],bytes=command[8];
  assert(w<119 && (write?command[2]:command[5])==129 && bytes && local>=0x75000 && local+bytes<=0x75800 && !((local^remote)&31));
  if(write){
   if(remote==0x74200 || remote==0x74000){
    assert(bytes==(remote==0x74200?256U:64U) && p.pc==TT_Q38_PEER_WAIT_LOAD);
    for(unsigned i=0;i<bytes/4;i++)p.memory[remote+i*4]=memory.at(local+i*4);
    if(remote==0x74200)for(unsigned i=0;i<64;i++)n.frame[i]=p.memory.at(remote+i*4);
    else for(unsigned i=0;i<16;i++)n.job[i]=p.memory.at(remote+i*4);
   }else{
    assert(remote==0x74100 && bytes==4 && memory.at(local)==p.memory.at(0x74144)+1);
    p.Prepare(memory);BeforeDoorbell(w);p.memory[remote]=memory.at(local);
   }
  }else if(remote==0x74140){
   assert(bytes==64);if(n.requested!=n.sequence || !n.verified)Advance(w);
   for(unsigned i=0;i<16;i++)memory[local+i*4]=p.memory.at(remote+i*4);
  }else if(remote==0x83f00){
   assert(n.verified && n.sequence==n.requested && (p.pc==TT_Q38_PEER_WAIT_LOAD || p.pc==TT_Q38_PEER_STOP));
   assert(bytes==256 || (n.op==1 && bytes==256+p.groups*64));
   for(unsigned i=0;i<64;i++)memory[local+i*4]=p.memory.at(remote+i*4);
   if(bytes>256)for(unsigned row=0;row<p.groups*16;row++)memory[local+256+row*4]=p.buffers[4096+row];
  }else{assert(bytes==4);memory[local]=PhysicalRegister(w,remote);}
  Retire(bytes,write);
 }
 void VerifyArithmetic(){
  const auto* m=fixture.graph->Manifest();assert(!pending && !unknown && boots==119);
  for(unsigned w=0;w<119;w++)assert(actual[w]->memory.at(0x74144)==PioneerQ38ProjectionGraphNextPeer(m,w)
   && node[w].sequence==actual[w]->memory.at(0x74144) && node[w].stopped==fixture.stop);
  if(fixture.stop)assert(stopJobs==119 && !producerJobs && !matrixJobs && !outputWrites);
  else{
   assert(producerJobs==m[44]/256 && matrixJobs==m[12] && lineReads==m[44]/256 && scaleWrites==1 && scaleReads==m[7]);
   for(unsigned row=0;row<m[45];row++)assert(outputCover[row]==1 && gddr.at(PioneerQ38ProjectionOutputOffset(m[42],m[43])/4+row)==expected[row]);
  }
  assert(memory.at(TT_Q38_PROJECTION_GRAPH_STATS+56*4)==1);runs++;
 }
};
#ifndef PIONEER_Q38_PROJECTION_COOPERATIVE_FIXTURE_ONLY
int main(int argc,char** argv){
 assert(argc==3 || (argc==4 && !strcmp(argv[3],"--wide")));(void)&Input;(void)&Prepare;(void)&Upload;(void)&Close;(void)&BankProof;
 setvbuf(stdout,nullptr,_IOLBF,0);auto root=std::make_unique<ArithmeticParent>(argv[1],argv[2]);
 std::vector<std::pair<unsigned,unsigned>> cases={{0,10},{3,6},{0,10}};
 if(argc==4){cases.push_back({0,2});cases.push_back({3,10});cases.push_back({0,4});}
 unsigned rows=0;for(unsigned i=0;i<cases.size();i++){
  if(i)root->Retarget(cases[i].first,cases[i].second,false);
  assert(root->Run());root->VerifyArithmetic();rows+=root->fixture.jobs->Outputs();
  printf("PASS ACTUAL parent+ALL119 retained peers projection%u layer%u role%u type%u K%u N%u ALL%u waves/FP32+FP64;NO host interstage calls/peer resets,NOT_NATIVE_TIME\n",i,root->fixture.layer,root->fixture.role,root->fixture.jobs->Type(),root->fixture.jobs->Inputs(),root->fixture.jobs->Outputs(),root->fixture.jobs->Waves());
 }
 root->Retarget(cases.back().first,cases.back().second,true);assert(root->Run());root->VerifyArithmetic();
#ifdef PIONEER_Q38_TEST_PROJECTION_FOOTPRINT
 uint64_t peerStores=0;for(const auto& p:root->actual){assert(p->observedL1Stores);peerStores+=p->observedL1Stores;}
 assert(root->observedL1Stores);
 printf("PASS fixed L1 snapshot DATA covers ACTUAL emitted CPU stores ROOT%llu/ALL119peers%llu;NO immutable code stores;NOT_NATIVE_MAPPING_OR_COMPLETE_CSR_TENSIX_STATE_OWNERSHIP\n",
  (unsigned long long)root->observedL1Stores,(unsigned long long)peerStores);
#endif
 printf("PASS OFFLINE actual full-projection parent ALL119boots ONCE/projections%zu/outputRows%u/producers%u/matrixJobs%u/instructions%llu/ALL119physicalSTOP;changed roles/inputs/widths/types%s,NO peer/Dst/Src/CSR/private-sequence reset;SYNTHETIC weights/transport_NOT_NATIVE_FULL_MODEL_OR_SPEED\n",cases.size(),rows,root->completedProducers,root->completedMatrices,(unsigned long long)root->matrixInstructions,argc==4?"8/12/14":"8ONLY");
}
#endif
