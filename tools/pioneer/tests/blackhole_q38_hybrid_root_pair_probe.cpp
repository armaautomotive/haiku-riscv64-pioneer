/* SPDX-License-Identifier: MIT */
// RV32 simulation entry only. Not a model coordinator or installable image.
#include "../PioneerQ38HybridRootPairs.h"
static volatile unsigned* Words(unsigned a){return reinterpret_cast<volatile unsigned*>(a);}
struct ProbeIO {
 unsigned Table(unsigned bank,unsigned i){return Words(0x77200+bank*16)[i];}
 unsigned Result(unsigned w,unsigned i){return Words(0x76000+w*32)[i];}
 unsigned ResultRow(unsigned w){return Words(0x77000)[w];}
 unsigned Route(unsigned bank){
  const unsigned routes[]={786,978,1170,785,977,1169,1361};return bank<7?routes[bank]:~0U;
 }
 void Output(unsigned w,unsigned i,unsigned value){Words(0x76000+w*32)[i]=value;}
};
extern "C" unsigned native_dram_band_main(){
 ProbeIO io;return PioneerQ38HybridRootPairs::Relocate(io,Words(0x74000)[0],Words(0x74000)[1],Words(0x74000)[2]);
}
