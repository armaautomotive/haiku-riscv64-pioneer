/* SPDX-License-Identifier: MIT */
// Actual QFG3 ROOT stats/counters,not synthetic receipt generation. Peer
// products are simulated here;actual arithmetic is separately qualified.
#define PIONEER_Q38_FFN_GRAPH_MACHINE_ONLY
#include "pioneer_q38_ffn_graph_image_test.cpp"
#include "pioneer_q38_ffn_residual_graph_fixture.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnResidualGraphProof.h"
static unsigned Checked(ResidualGraphFixture<GraphRootMachine>& root,const uint32_t* before){
 uint32_t s[64],after[8];
 for(unsigned i=0;i<64;i++)s[i]=root.memory.at(TT_Q38_FFN_GRAPH_STATS+i*4);
 for(unsigned i=0;i<8;i++)after[i]=root.counts[i==6?2:i];
 const auto* m=root.fixture.manifest.data();
 assert(PioneerQ38FfnResidualGraphManifest(m,root.fixture.source.data())
  && PioneerQ38FfnResidualGraphReceipt(s,m,before,after));unsigned refused=0;
 for(unsigned i=0;i<64;i++)if(i!=10 && i!=11){s[i]^=1;assert(!PioneerQ38FfnResidualGraphReceipt(s,m,before,after));s[i]^=1;refused++;}
 for(unsigned i=0;i<8;i++){after[i]^=1;assert(!PioneerQ38FfnResidualGraphReceipt(s,m,before,after));after[i]^=1;refused++;}
 for(unsigned field:{10U,11U}){unsigned saved=s[field];s[field]=0;assert(!PioneerQ38FfnResidualGraphReceipt(s,m,before,after));s[field]=UINT32_MAX;
  assert(!PioneerQ38FfnResidualGraphReceipt(s,m,before,after));s[field]=saved;refused+=2;}
 return refused;
}
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);unsigned passed=0,refused=0;
 for(bool wide:{false,true})for(unsigned bank=0;bank<7;bank++){
  ResidualGraphFixture<GraphRootMachine> root(argv[1],wide,bank,(bank&1)!=0,(bank&2)!=0,bank);
  uint32_t before[8];for(unsigned i=0;i<8;i++)before[i]=root.counts[i==6?2:i];
  assert(root.RunGraph());root.VerifyGraph();root.VerifyResidual();refused+=Checked(root,before);passed++;
  for(unsigned i=0;i<8;i++)before[i]=root.counts[i==6?2:i];root.StopRequest();assert(root.RunGraph());root.VerifyGraph();
  refused+=Checked(root,before);passed++;
 }
 printf("PASS ACTUAL QFG3 ROOT receipt positive%u/mutations%u/ALL7banks/full119andnarrow/mixedQ4-Q6/roworders/modulo-counters/9stage residual+STOP/independent BEFORE+AFTER;NO native lifetime/physical/TPS claim\n",passed,refused);
}
