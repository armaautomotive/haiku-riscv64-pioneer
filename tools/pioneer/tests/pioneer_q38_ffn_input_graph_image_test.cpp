/* SPDX-License-Identifier: MIT */
// ACTUAL QFG2 root instructions;peer/math/physical endpoints SIMULATED.
// Never a native initial-producer/source/lifetime or speed grant.
#define PIONEER_Q38_FFN_GRAPH_MACHINE_ONLY
#include "pioneer_q38_ffn_graph_image_test.cpp"
struct InputGraphMachine:GraphRootMachine {
 unsigned issued=0,completed=0,drop=0,corrupt=0,mutateSource=0,mutateManifest=0,mutateInput=0;
 unsigned faultPacket=0,inputReads=0,inputWrites=0;
 bool aliasBeforeInput=false;
 std::vector<uint32_t> expectedInput;
 explicit InputGraphMachine(const char* image,unsigned bank=0):GraphRootMachine(image,true,bank,(bank&1)!=0,(bank&2)!=0,bank) {
  for(unsigned i=0;i<5120;i++)expectedInput.push_back(memory.at(0x80000+i*4));
  std::fill(gddr.begin(),gddr.begin()+5120,0x7fc00001); // NOT prepopulated by test
 }
 bool InputCommand()const {
  bool write=command[7]==0x212;unsigned route=write?command[5]:command[2],remote=write?command[3]:command[0];
  return route==fixture.manifest[8] && remote>=fixture.manifest[4] && remote-fixture.manifest[4]<20480;
 }
 void Store(uint32_t a,uint32_t value)override {
  if(a==0xffb20040 && InputCommand()) {
   assert(!stages[1] && !stages[3] && !stages[4] && issued<4);
   issued++;if(issued==drop){faultAt=packets+1;faultPacket=packets+1;}
  }
  GraphRootMachine::Store(a,value);
 }
 void Packet()override {
  bool input=InputCommand(),write=command[7]==0x212;
  unsigned remote=write?command[3]:command[0],local=write?command[0]:command[3],bytes=command[8];
  unsigned route=write?command[5]:command[2];
  if(input){unsigned index=issued-1,offset=index<2?0:16384;
   assert(issued==completed+1 && write==!(index&1) && remote==fixture.manifest[4]+offset
    && bytes==(offset?4096U:16384U) && local==(write?0x80000+offset:0x88000));
  }
  GraphRootMachine::Packet();
  if(aliasBeforeInput && !stages[1] && !stages[3] && !stages[4] && route==Route(119)
   && remote==0xffb20000+BlackholeDramSourceOffset(15))faultOwnAliasScan=ownCounterScans+2;
  if(!input)return;
  completed++;if(write)inputWrites++;else inputReads++;
  if(issued==corrupt){assert(!write);memory[local+(bytes/4-1)*4]^=1;faultPacket=packets;}
  if(issued==mutateSource){rootSource[11]^=1;faultPacket=packets;}
  if(issued==mutateManifest){memory[TT_Q38_FFN_GRAPH_MANIFEST+40*4]^=1;faultPacket=packets;}
  if(issued==mutateInput){assert(write);memory[0x80000+bytes-4]^=1;faultPacket=packets+1;}
 }
 void VerifyInput() {
  assert(issued==4 && completed==4 && inputReads==2 && inputWrites==2);
  for(unsigned i=0;i<5120;i++)assert(gddr[i]==expectedInput[i]);
  const uint32_t receipt[]={0x32494651,20480,2,2,fixture.manifest[13],fixture.manifest[40],fixture.manifest[41]};
  assert(memory.at(TT_Q38_FFN_GRAPH_STATS+4)==2 && memory.at(TT_Q38_FFN_GRAPH_STATS+49*4)==8);
  for(unsigned i=0;i<7;i++)assert(memory.at(TT_Q38_FFN_GRAPH_STATS+(57+i)*4)==receipt[i]);
 }
 void Failed() {
  assert(!RunGraph() && unknown && !stages[1] && !stages[3] && !stages[4]);
  if(faultPacket)assert(packets==faultPacket);
  Inert();
 }
};
int main(int argc,char** argv) {
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned positive=0,faults=0,maxStack=0;
 for(unsigned bank=0;bank<7;bank++) {
  InputGraphMachine root(argv[1],bank);assert(root.RunGraph());root.VerifyInput();root.VerifyGraph();positive++;
  maxStack=std::max(maxStack,0x70000-root.minStack);
  root.StopRequest();assert(root.RunGraph());root.VerifyGraph();
  for(unsigned i=57;i<64;i++)assert(!root.memory.at(TT_Q38_FFN_GRAPH_STATS+i*4)); // STOP never copies input
  assert(root.issued==4 && root.completed==4);positive++;
 }
 for(unsigned at:{0U,255U,4095U,4096U,5119U})for(uint32_t bits:{0x7f800000U,0xff800000U,0x7fc00001U}) {
  InputGraphMachine root(argv[1]);root.memory[0x80000+at*4]=bits;root.Failed();assert(!root.issued);faults++;
 }
 for(unsigned packet=1;packet<=4;packet++){InputGraphMachine root(argv[1]);root.drop=packet;root.Failed();
  assert(root.issued==packet && root.completed+1==packet && root.pending);faults++;}
 for(unsigned packet:{2U,4U}){InputGraphMachine root(argv[1]);root.corrupt=packet;root.Failed();faults++;}
 for(unsigned packet=1;packet<=4;packet++)for(bool source:{false,true}) {
  InputGraphMachine root(argv[1]);if(source)root.mutateSource=packet;else root.mutateManifest=packet;root.Failed();faults++;
 }
 {InputGraphMachine root(argv[1]);root.mutateInput=1;root.Failed();assert(root.issued==2);faults++;}
 for(uint32_t accepted:{0U,UINT32_MAX,UINT32_MAX-100U}) {
  InputGraphMachine root(argv[1]);root.counts[2]=accepted;root.aliasBeforeInput=true;root.Failed();
  assert(root.ownAliasFaultObserved && !root.issued && !root.ownReadsAfterFault && !root.commandWritesAfterFault);faults++;
 }
 // Different ABI/marker refuses as configuration BEFORE any packet.
 for(unsigned which=0;which<2;which++) {
  InputGraphMachine root(argv[1]);root.fixture.manifest[which]=which?1:0x31474651;
  for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++)root.memory[TT_Q38_FFN_GRAPH_MANIFEST+i*4]=root.fixture.manifest[i];
  assert(!root.RunGraph() && !root.unknown && !root.packets && !root.issued);faults++;
 }
 printf("PASS OFFLINE ACTUAL QFG2 initial-F32 producer+complete FFN root:positive%u/faults%u/maxStack%u;ALL7banks/high-edge/reversedrows/119peers,20480B actual copy+readback from owned L1 into POISONED GDDR,receipt only after exact completion,STOP never copies,nonfinite/full-source/short-packet/pending/drop/corrupt/source/manifest/late-source/oldABI failures inert before ANY peer computation;SYNTHETIC_PEER_VALUES_NOT_NATIVE_SOURCE_OWNER_OR_SPEED\n",positive,faults,maxStack);
}
