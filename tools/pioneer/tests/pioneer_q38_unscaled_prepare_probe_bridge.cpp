/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_GDN_PREPARE_MACHINE_ONLY
#include "pioneer_q38_gdn_prepare_image_test.cpp"
#include "pioneer_q38_silu_precision_probe_bridge.h"
void ExecuteUnscaledPrepareProbe(const char* image,unsigned half,const uint32_t* input,uint32_t* output){
 assert(image && half<96 && input && output);PrepareMachine machine(image,half);
 for(unsigned i=0;i<324;i++)machine.memory[0x80000+i*4]=machine.input[i]=input[i];
 assert(machine.Run() && machine.localWrites==322 && machine.descriptorWrites==5);
 for(unsigned i=0;i<322;i++)output[i]=machine.memory.at(GL::Query+i*4);
}
