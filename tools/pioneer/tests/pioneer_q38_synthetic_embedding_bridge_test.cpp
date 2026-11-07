/* SPDX-License-Identifier: MIT */
// Synthetic fixture mapping/refusals only; not a real model or device test.
#define PIONEER_Q38_EMBEDDING_RAW_TEST_BRIDGE 1
#include "pioneer_q38_embedding_raw_test_bridge.h"
#include "../PioneerQ38GlobalFeedbackLayout.h"
#include <cassert>
#include <cstring>

int main(int argc,char** argv){
 assert(argc==2);Q38EmbeddingRawTestBridge source;uint8_t tile[2304]={},block[144]={};
 assert(!source.Read(0,0,tile,sizeof(tile)) && !source.OriginalBlock(248319,0,block,sizeof(block)));
 assert(source.Open(argv[1]) && source.Ready() && !source.Open(argv[1]));
 uint32_t geometry[8];assert(PioneerQ38GlobalFeedbackLayout::Describe(248319,0,0,0xc0000000U,geometry));
 const unsigned route=PioneerQ38ProjectionRoute(geometry[4]);
 assert(!source.Read(route^1U,geometry[5],tile,sizeof(tile)) && !source.Read(route,geometry[5]+1,tile,sizeof(tile))
  && !source.Read(route,geometry[5],tile,sizeof(tile)-1) && !source.Read(route,geometry[5]+20*2304,tile,sizeof(tile)));
 assert(!source.OriginalBlock(248318,0,block,sizeof(block)) && !source.OriginalBlock(248319,20,block,sizeof(block)));
 for(unsigned n=0;n<20;n++){
  assert(source.Read(route,geometry[5]+n*2304,tile,sizeof(tile)) && source.OriginalBlock(248319,n,block,sizeof(block)));
  for(unsigned i=0;i<144;i++)assert(tile[i*16+geometry[6]]==block[i]);
 }
 source.Revoke();assert(!source.Ready() && !source.Read(route,geometry[5],tile,sizeof(tile))
  && !source.OriginalBlock(248319,0,block,sizeof(block)) && !source.Open(argv[1]));
}
