/* SPDX-License-Identifier: MIT */
// Actual QFG3 instructions;peer products/physical fabric SYNTHETIC.
// Full actual peer arithmetic has a separate cooperative joined test.
#define PIONEER_Q38_FFN_GRAPH_MACHINE_ONLY
#include "pioneer_q38_ffn_graph_image_test.cpp"
#include "pioneer_q38_ffn_residual_graph_fixture.h"
using ResidualRoot=ResidualGraphFixture<GraphRootMachine>;
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned positives=0,faults=0,maxStack=0;
 for(unsigned bank=0;bank<7;bank++){
  ResidualRoot root(argv[1],true,bank,(bank&1)!=0,(bank&2)!=0,bank);
  assert(root.RunGraph());root.VerifyGraph();root.VerifyResidual();positives++;
  maxStack=std::max(maxStack,0x70000-root.minStack);
  auto output=root.gddr;unsigned packets=root.issued,stores=root.sumStores;
  root.StopRequest();assert(root.RunGraph());root.VerifyGraph();positives++;
  assert(root.gddr==output && root.issued==packets && root.sumStores==stores);
  for(unsigned i=57;i<64;i++)assert(!root.memory.at(TT_Q38_FFN_GRAPH_STATS+i*4));
 }
 for(unsigned lane:{0U,4095U,4096U,5119U})for(uint32_t value:{0x7f800000U,0xff800000U,0x7fc00001U,1U,0x477fe001U}){
  ResidualRoot root(argv[1]);root.gddr.at(0x18000/4+lane)=value;root.Failed();
  assert(root.issued==2 && !root.stages[1] && !root.stages[3] && !root.stages[4] && !root.writes);faults++;
 }
 for(unsigned packet=1;packet<=6;packet++){
  ResidualRoot root(argv[1]);root.drop=packet;root.Failed();assert(root.issued==packet && root.completed+1==packet && root.pending);faults++;
 }
 for(unsigned packet=1;packet<=6;packet++)for(unsigned mode=0;mode<3;mode++){
  ResidualRoot root(argv[1]);if(!mode)root.mutateSource=packet;else if(mode==1)root.mutateManifest=packet;else root.badCounterAt=packet;
  root.Failed();assert(root.issued==packet && root.completed==packet);faults++;
 }
 for(unsigned packet:{3U,4U})for(uint32_t value:{0x7fc00001U,0x7f800000U,1U,0x477fe001U}){
  ResidualRoot root(argv[1]);root.mutateDown=packet;root.downValue=value;root.Failed();
  assert(root.issued==4 && !root.writes);for(unsigned i=0;i<5120;i++)assert(root.gddr[i]==root.savedInput[i]);faults++;
 }
 // Individually valid finite inputs whose SUM leaves the SAME RMS domain.
 {ResidualRoot root(argv[1]);root.gddr.at(0x18000/4+5119)=0x477fe000;root.mutateDown=4;root.downValue=0x477fe000;
  root.Failed();assert(!root.writes && root.sumStores==5119);faults++;}
 for(unsigned word=0;word<2;word++)for(unsigned old:{1U,2U}){
  ResidualRoot root(argv[1]);root.fixture.manifest[word]=word?old:old==1?0x31474651U:0x32474651U;
  for(unsigned i=0;i<TT_Q38_FFN_GRAPH_WORDS;i++)root.memory[TT_Q38_FFN_GRAPH_MANIFEST+i*4]=root.fixture.manifest[i];
  assert(!root.RunGraph() && !root.unknown && !root.packets);faults++;
 }
 printf("PASS OFFLINE ACTUAL QFG3 root:positive%u/faults%u/maxStack%u;ALL7banks,raw5120 preserved BEFORE overlapping gate/up,completeFFN then5120F32 residual oncard/ALL-domain BEFORE hidden write/ALL119STOP/no host staging/UNKNOWN inert;SYNTHETIC_PEER_PRODUCTS_NOT_NATIVE_OR_MODEL_SPEED\n",positives,faults,maxStack);
}
