/* SPDX-License-Identifier: MIT */
// Template instantiation ONLY. The actual native adapter is still absent.
#define PIONEER_Q38_RESIDENT_OUTPUT_OWNER 1
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_PROJECTION_RESULT_STASH 1
#include "pioneer_q38_projection_execution_kernel_compile.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputOwner.h"
struct ResidentOutputCompileIO:ProjectionExecutionCompileIO {
 void ResidentOutputRetainHostOwners();
 bool ResidentOutputExecutionScope(const PioneerQ38ResidentOutputOwner&,const PioneerQ38ProjectionGraphOwner&,const PioneerQ38ResidentOutputMemory&,
  const PioneerQ38ProjectionMutableBanks&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool ResidentOutputMemoryScope(const PioneerQ38ResidentOutputMemory&,const PioneerQ38CanonicalModelPlan&,const PioneerQ38GdnIdentity&,const uint8_t*,unsigned);
 bool ResidentOutputMemoryMappingStable(unsigned);
 bool ResidentOutputMemoryReadChecked(unsigned,uint32_t,uint32_t&);bool ResidentOutputMemoryWrite(unsigned,uint32_t,uint32_t);
};
extern "C" bool pioneer_resident_output_owner_compile(ResidentOutputCompileIO& io,PioneerQ38ProjectionGraphOwner& producer,
 PioneerQ38ResidentOutputOwner& heap,PioneerQ38ResidentOutputMemory& memory,PioneerQ38ResidentOutputResult& result){
 return heap.Prepare(io,producer,memory,result) && heap.Run(io,result) && heap.Close(io,result);
}
