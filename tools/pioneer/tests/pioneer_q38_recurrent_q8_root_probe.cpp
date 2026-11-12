/* SPDX-License-Identifier: MIT */
#define PIONEER_ATTENTION_FREESTANDING 1
#define PIONEER_Q38_RECURRENT_Q8_MATRIX_ROOT_INSPECTION 1
#include "../PioneerQ38RecurrentQ8MatrixRootJobs.h"
struct ProbeIO {
 volatile const uint32_t* table;volatile uint32_t* output;
 unsigned Table(unsigned bank,unsigned field){return table[bank*3+field];}
 void Output(unsigned worker,unsigned field,unsigned value){output[worker*4+field]=value;}
};
extern "C" unsigned q38_recurrent_q8_root_probe(const uint32_t* table,uint32_t* output,unsigned layer,unsigned role){
 ProbeIO io={table,output};return PioneerQ38RecurrentQ8MatrixRootJobs::Build(io,layer,role)?1:0;
}
