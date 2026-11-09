/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_BFP4_QUALITY_DIAGNOSTIC 1
#include "pioneer_q38_qgw8_cpu_layer_snapshot.h"
#include <cassert>
#include <cstdio>
#include <string>
int main(int argc,char** argv){
 assert(argc==2 && argv[1][0]=='/');
 using namespace Q38Qgw8CpuLayerSnapshot;
 std::vector<uint32_t> source((Layers+1)*Width),out(1,17);
 for(unsigned layer=0;layer<=Layers;layer++)for(unsigned i=0;i<Width;i++)source[layer*Width+i]=0x3f000000U+layer;
 assert(Build(source,out) && out.size()==Words);
 for(unsigned layer=0;layer<Layers;layer++)assert(out[layer*Width]==0x3f000001U+layer);
 std::string path=std::string(argv[1])+"/converted-layers.bin";
 assert(SaveNew(path.c_str(),out) && !SaveNew(path.c_str(),out));
 assert(!Build(source,source));
 const auto before=out;source[Width]=0x7f800000U;
 assert(!Build(source,out) && out==before);
 source.pop_back();assert(!Build(source,out) && out==before);
 out[0]=0x7fc00000U;assert(!SaveNew((path+"-bad").c_str(),out));
 assert(!SaveNew("relative",before));
 puts("PASS CPU-only converted layer boundaries/exclusive file/nonfinite/alias/shape refusals; NO DEVICE");
}
