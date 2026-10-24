/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_GATED_NORM_MACHINE_ONLY
#include "pioneer_q38_gdn_gated_norm_image_test.cpp"
#include "pioneer_q38_gdn_gated_norm_bridge.h"
void ExecuteGdnGatedNormLocal(const char* image,unsigned half,const uint32_t* raw,const uint32_t* norm,const uint32_t* z,uint32_t* output){
 assert(raw && norm && z && output && half<96);
 GatedNormMachine machine(image,half);
 for(unsigned i=0;i<128;i++){machine.input[i]=raw[i];machine.input[128+i]=norm[i];machine.input[256+i]=z[i];}
 machine.SetInputs();assert(machine.Run() && machine.writes==64 && machine.sealWrites==5);
 for(unsigned i=0;i<64;i++)output[i]=machine.memory.at(0x8ea00+i*4);
}
