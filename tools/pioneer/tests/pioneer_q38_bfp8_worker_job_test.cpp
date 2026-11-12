/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38Bfp8WorkerJob.h"
#include "../PioneerQ38Bfp4WorkerJob.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
using W=PioneerQ38Bfp8WorkerJob;using L=W::L;
static W::Job Make(unsigned worker,unsigned width){
 W::Job j={{W::Magic,W::Version,width,0x04000000,8,786,worker,W::Existing::Peer(worker),PioneerQ38Bf16Wire::Magic,
  W::RootInput,W::RootRoute,7,W::Output,0,0,0}};return j;
}
int main(){
 assert(!W::Shape(Make(1,6144))); // Experiment must not widen the existing worker.
 unsigned cases=0;
 for(unsigned worker=1;worker<120;worker++)for(unsigned width:{5120U,17408U}){
  auto j=Make(worker,width);assert(W::Shape(j));const unsigned tiles=width/32,bandBytes=tiles*1088;
  for(unsigned band=0;band<8;band++)for(unsigned tile:{0U,tiles-1})for(unsigned operand=0;operand<2;operand++){
   W::Read p={};assert(W::Packet(j,band,operand,tile,p));
   assert(p.source==(operand?W::RootInput+tile*64:j.words[3]+band*bandBytes+tile*1088));
   assert(p.destination==(operand?L::Input+tile*L::InputStride:L::Weight+tile*L::WeightStride));
   assert(p.bytes==(operand?64U:1088U) && p.route==(operand?W::RootRoute:j.words[5]));cases++;
  }
 }
 auto base=Make(1,5120);assert(W::Shape(base));
 for(unsigned word=0;word<W::Words;word++){
  auto bad=base;bad.words[word]=UINT32_MAX;W::Read untouched={1,2,3,4};
  assert(!W::Shape(bad) && !W::Packet(bad,0,0,0,untouched) && untouched.source==1 && untouched.destination==2 && untouched.bytes==3 && untouched.route==4);
 }
 base.words[0]=PioneerQ38Bfp4WorkerJob::Magic;assert(!W::Shape(base));
 auto edge=Make(1,17408);edge.words[3]=0xc4000000U-8U*591872U;assert(W::Shape(edge));
 edge.words[3]+=64;assert(!W::Shape(edge));
 printf("PASS BFP8 worker job: %u packet cases/all119 peers/both widths/all bands, malformed words, format separation and GDDR bounds; NO DEVICE\n",cases);
}
