/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION 1
#include "../PioneerQ38Bfp8WorkerJob.h"
#include <cassert>
#include <cstdio>
int main(){
 using W=PioneerQ38Bfp8WorkerJob;using L=W::L;
 for(unsigned worker=1;worker<120;worker++){
  W::Job j={{W::Magic,W::Version,6144,0x04000000,8,786,worker,W::Existing::Peer(worker),
   PioneerQ38Bf16Wire::Magic,W::RootInput,W::RootRoute,7,W::Output,0,0,0}};
  assert(W::Shape(j) && L::Tiles(6144)==192 && L::BandBytes(6144)==208896);
  for(unsigned band=0;band<8;band++)for(unsigned tile=0;tile<192;tile++)for(unsigned operand=0;operand<2;operand++){
   W::Read p={};assert(W::Packet(j,band,operand,tile,p));
   assert(p.source==(operand?W::RootInput+tile*64:j.words[3]+band*208896+tile*1088));
   assert(p.destination==(operand?L::Input+tile*L::InputStride:L::Weight+tile*L::WeightStride));
   assert(p.bytes==(operand?64U:1088U) && p.route==(operand?W::RootRoute:j.words[5]));
  }
  j.words[3]=0xc4000000U-8U*208896U;assert(W::Shape(j));j.words[3]+=64;assert(!W::Shape(j));
 }
 std::puts("PASS inspection 6144-wide BFP8 packet geometry; all peers, bands and tiles; NO DEVICE");
}
