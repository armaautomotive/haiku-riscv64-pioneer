/* SPDX-License-Identifier: MIT */
// ACTUAL unchanged QFG2 ROOT final sealing,simulated peers/physical fabric.
// Faults alter memory ONLY after ALL357 learned matrix jobs were issued.
// No native source/cookie/compute authority and NO performance claim.
#define PIONEER_Q38_FFN_GRAPH_MACHINE_ONLY
#include "pioneer_q38_ffn_graph_image_test.cpp"
struct FinalSealFault:GraphRootMachine {
 unsigned word,atPackets=0,atReads=0;bool shadow,injected=false;
 FinalSealFault(const char* image,unsigned selected,bool copy):GraphRootMachine(image),word(selected),shadow(copy){}
 uint32_t Load(uint32_t a)override{
  // Original manifest reads occur in Sealed(),not the matrix descriptor
  // consumer. Inject at the exact original/shadow comparison for this word.
  if(!injected && stages[1]==3*119 && a==TT_Q38_FFN_GRAPH_MANIFEST+word*4){
   assert(!pending && !unknown && memory.at(TT_Q38_FFN_GRAPH_STATS+56*4)==0);
   memory[(shadow?TT_Q38_FFN_GRAPH_SHADOW:TT_Q38_FFN_GRAPH_MANIFEST)+word*4]^=1;
   injected=true;atPackets=packets;atReads=nocReads;
  }
  return GraphRootMachine::Load(a);
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);
 {GraphRootMachine root(argv[1]);assert(root.RunGraph());root.VerifyGraph();
  assert(root.memory.at(TT_Q38_FFN_GRAPH_STATS+56*4)==1 && root.pc==TT_Q38_DISPATCH_PARK);}
 unsigned faults=0;
 for(bool shadow:{false,true})for(unsigned word:{0U,40U,42U,TT_Q38_FFN_GRAPH_WORDS-1U}){
  FinalSealFault root(argv[1],word,shadow);
  assert(!root.RunGraph() && root.injected && root.unknown && root.pc==TT_Q38_DISPATCH_FAULT_PARK
   && root.packets==root.atPackets && root.nocReads==root.atReads
   && !root.memory.at(TT_Q38_FFN_GRAPH_STATS+56*4));
  root.Inert();faults++;
  printf("PASS ACTUAL QFG2 late-seal fault%u shadow%u word%u;no receipt/following NoC packet or read/STOP/retry;SIMULATED_ENDPOINTS\n",faults,unsigned(shadow),word);
 }
 printf("PASS ACTUAL unchanged QFG2 final sealing1positive/%ufaults BEFORE completion receipt;NO_NATIVE_OR_SPEED_AUTHORITY\n",faults);
}
