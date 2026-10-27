/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_SIGMOID_PRECISION_MACHINE_ONLY
#include "pioneer_q38_sigmoid_precision_image_test.cpp"
#include "pioneer_q38_sigmoid_precision_probe_bridge.h"
void ExecuteSigmoidPrecisionProbe(const char* image,const uint32_t* input,uint32_t* output){
 (void)&SigmoidReference;assert(image && input && output);SigmoidPrecisionMachine machine(image,0);
 for(unsigned i=0;i<64;i++)machine.memory[0x80000+i*4]=machine.input[i]=input[i];
 assert(machine.Run() && machine.outputWrites==64);
 for(unsigned i=0;i<64;i++)output[i]=machine.memory.at(0x81000+i*4);
}
