/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_factored_q4_compare_machine.h"
#include "../PioneerQ38Q4FactoredTraceReference.h"
int main(int argc,char** argv){
 assert(argc==2);FactoredCompareData data(5120,0);
 auto vm=std::make_unique<FactoredCompareRV32>(argv[1],5120,true);data.Stage(vm->bus);
 assert(vm->Execute());PioneerQ38Q4FactoredTraceReference reference;
 for(unsigned block=0;block<20;block++)assert(reference.Block(data.blocks[block].original.data(),data.blocks[block].input.data(),block));
 unsigned expected[64];assert(reference.Export(expected));
 for(unsigned i=0;i<64;i++)assert(vm->ReadL1(SL::Output+i*4)==expected[i]);
 for(unsigned i=0;i<16;i++)assert(vm->ReadL1(SL::Output+256+i*4)==0);
 printf("PASS_COMPILED_TRACE K5120 originalData_final32_block20_stage4_first8_exact actualRV32_sharedPhysicalDst_NoC_PACK_FPU_SFPU observationReads%u SP%u_touched%u;NO_CLOCK_NATIVE_OWNER_HARDWARE_OR_SPEED_PROOF\n",vm->diagnosticReads,0x70000-vm->minSP,0x70000-vm->minStore);
 static_cast<void>(&NoMore);
}
