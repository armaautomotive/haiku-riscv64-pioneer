/* SPDX-License-Identifier: MIT */
#define PIONEER_NATIVE_DRAM_LOADER_TEST_ONLY
#include "pioneer_native_dram_band_loader_test.cpp"
#include "../PioneerNativeBfp4DramBand.h"
using B4=PioneerNativeBfp4DramBand;
using D4=PioneerNativeBfp4DramBandLayout;

int main(){
 unsigned passed=0,refused=0;
 for(unsigned k:{5120U,6144U,17408U}){
  for(unsigned tile=0;tile<D4::Tiles(k);tile++)for(unsigned operand=0;operand<2;operand++){
   D4::Read p={};assert(D4::Packet(k,operand,tile,p));
   assert(!(p.source&63) && !(p.destination&63) && !(p.bytes&63));
   assert(p.bytes==(operand?64U:576U));
  }
  for(unsigned x:{17U,18U})for(unsigned y:{12U,15U,18U,21U}){
   LoaderIO<D4> io(k,x|(y<<6));B4::Loader loader;assert(loader.Run(io,k,io.route));io.Exact();
   unsigned before=io.events;assert(!loader.Run(io,k,io.route) && before==io.events);passed++;
  }
  Events canonical,translated;assert(PioneerQ38NativeBfp4Band::Run(canonical,k));assert(B4::Compute(translated,k));
  assert(canonical.all.size()==translated.all.size());unsigned weights=0,inputs=0;bool active=false;
  for(unsigned i=0;i<canonical.all.size();i++){
   auto old=canonical.all[i];auto got=translated.all[i];assert(old.kind==got.kind && old.index==got.index);
   if(old.kind==1 && old.index==112 && old.value==0x00200015)active=true;
   if(old.kind==1 && old.index==76)old.value=D4::Weight/16-1+(active?weights++*(D4::WeightStride/16):0);
   if(old.kind==1 && old.index==124)old.value=D4::Input/16-1+(active?inputs++*(D4::InputStride/16):0);
   assert(old.value==got.value);
  }
  assert(weights==D4::Tiles(k) && inputs==weights);passed++;
 }
 for(unsigned fault=1;fault<=12;fault++){
  LoaderIO<D4> io(5120);io.fault=fault;B4::Loader loader;assert(!loader.Run(io,5120,io.route));
  unsigned before=io.events;assert(!loader.Run(io,5120,io.route) && io.events==before && !io.scalarPayloadReads);refused++;
 }
 for(unsigned drop=0;drop<10;drop++){
  LoaderIO<D4> io(5120);io.dropped=drop;B4::Loader loader;assert(!loader.Run(io,5120,io.route) && !io.issues);refused++;
 }
 for(unsigned bad:{0U,32U,5119U,5121U,17409U,~0U}){
  LoaderIO<D4> io(5120);B4::Loader loader;assert(!loader.Run(io,bad,io.route) && !io.events);
  Events none;assert(!B4::Compute(none,bad) && none.all.empty());refused++;
 }
 for(unsigned bad:{0U,129U,16U|(12U<<6),17U|(14U<<6),~0U}){
  LoaderIO<D4> io(5120);B4::Loader loader;assert(!loader.Run(io,5120,bad) && !io.events);refused++;
 }
 D4::Read preserved={1,2,3};
 assert(!D4::Packet(5120,2,0,preserved) && preserved.source==1 && preserved.destination==2 && preserved.bytes==3);
 assert(!D4::Packet(5120,0,160,preserved) && preserved.source==1);refused+=2;
 {
  Events out;B4::Descriptors<Events> adapter(out,5120);adapter.Config(112,0x00200015);
  auto before=out.all.size();adapter.Config(76,0);adapter.Op(1);adapter.Drain();adapter.Config(1,2);
  assert(!adapter.Complete() && out.all.size()==before);refused++;
 }
 printf("PASS OFFBOARD BFP4 GDDR loader:%u valid/%u refused;3 widths,8 routes,whole L1 and64B headers,aligned packed reads,16-command queue,exact physical BFP4 operation sequence except checked descriptors;NO physical execution/model/speed claim\n",passed,refused);
}
