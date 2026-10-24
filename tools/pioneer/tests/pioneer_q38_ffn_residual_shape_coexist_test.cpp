/* SPDX-License-Identifier: MIT */
// The OLD QFG2 ABI and NEW QFG3 ABI coexist without changing either marker.
#define PIONEER_Q38_FFN_GRAPH_INPUT_ROOT 1
#include "pioneer_q38_ffn_graph_fixture.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38FfnResidualGraphShape.h"
int main(){
 static_assert(TT_Q38_FFN_GRAPH_MARKER==0x32474651 && TT_Q38_FFN_RESIDUAL_GRAPH_MARKER==0x33474651,"never globally retag old host-input grammar");
 unsigned positive=0,refused=0;
 for(unsigned bank=0;bank<7;bank++)for(bool wide:{false,true})for(unsigned layer=0;layer<64;layer++){
  FfnGraphFixture old(bank,(bank&1)!=0,wide,(bank&2)!=0,bank);old.manifest[42]=layer;
  auto next=old.manifest;next[0]=TT_Q38_FFN_RESIDUAL_GRAPH_MARKER;next[1]=3; // TEST metadata only,not producer admission
  assert(PioneerQ38FfnGraphManifest(old.manifest.data(),old.source.data())
   && PioneerQ38FfnResidualGraphManifest(next.data(),old.source.data()));positive+=2;
  assert(!PioneerQ38FfnResidualGraphManifest(old.manifest.data(),old.source.data())
   && !PioneerQ38FfnGraphManifest(next.data(),old.source.data()));refused+=2;
  for(unsigned w=0;w<119;w++)assert(PioneerQ38FfnGraphNextPeer(old.manifest.data(),w)==PioneerQ38FfnResidualGraphNextPeer(next.data(),w));
  for(unsigned i=0;i<64;i++)if(i!=0 && i!=1){
   old.manifest[i]^=1;next[i]^=1;
   assert(PioneerQ38FfnGraphManifest(old.manifest.data(),old.source.data())==PioneerQ38FfnResidualGraphManifest(next.data(),old.source.data()));
   old.manifest[i]^=1;next[i]^=1;
  }
 }
 printf("PASS coexist originalQFG2/newQFG3 DATA positives%u/crossABIrefusals%u/896bank-layer-workforce shapes+header mutation equivalence/private typed producer still REQUIRED;NO native/CPU staging grant\n",positive,refused);
}
