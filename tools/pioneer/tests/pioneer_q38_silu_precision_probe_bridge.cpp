/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SILU_PRECISION_MACHINE_ONLY
#include "pioneer_q38_silu_precision_image_test.cpp"
#include "pioneer_q38_silu_precision_probe_bridge.h"
void ExecuteSiluPrecisionProbe(const char* image,const uint32_t* input,uint32_t* output){
 assert(image && input && output);SiluPrecisionMachine machine(image,0);
 for(unsigned i=0;i<64;i++)machine.memory[0x80000+i*4]=machine.input[i]=input[i];
 assert(machine.Run() && machine.outputWrites==64);
 for(unsigned i=0;i<64;i++)output[i]=machine.memory.at(0x81000+i*4);
}
