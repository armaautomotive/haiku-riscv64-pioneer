/* SPDX-License-Identifier: MIT */
// Real RISC-V kernel ABI, offline DATA only. No owner/route/device calls.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextGdnOutputImages.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextGdnOutputGraphProof.h"
extern "C" bool pioneer_next_gdn_output_input(const uint32_t* line,const uint32_t* descriptor){
 return PioneerQ38NextGdnOutputInputLine(line,descriptor);
}
extern "C" bool pioneer_next_gdn_output_input_proof(const uint32_t* proof,const uint32_t* job,const uint32_t* descriptor,
 const uint32_t* source,unsigned worker,unsigned self,const uint32_t* before,const uint32_t* after){
 return PioneerQ38NextGdnOutputInputProof(proof,job,descriptor,source,worker,self,before,after);
}
extern "C" bool pioneer_next_gdn_output_gather(const uint32_t* line,const uint32_t* descriptor,unsigned chunk,const uint32_t* source){
 return PioneerQ38NextGdnOutputGatherLine(line,descriptor,chunk,source);
}
extern "C" bool pioneer_next_gdn_output_manifest(const uint32_t* manifest,const uint32_t* sources){
 return PioneerQ38NextGdnOutputGraphManifest(manifest,sources);
}
extern "C" bool pioneer_next_gdn_output_matrix(const uint32_t* d,const uint32_t* h,const uint32_t* banks,const uint32_t parts[119][8],const uint32_t* rows){
 return PioneerQ38NextGdnOutputMatrixShape(d,h,banks,parts,rows);
}
extern "C" bool pioneer_next_gdn_output_receipt(const uint32_t* s,const uint32_t* m,const uint32_t* before,const uint32_t* after){
 return PioneerQ38NextGdnOutputGraphReceipt(s,m,before,after);
}
extern "C" uint32_t pioneer_next_gdn_output_program(unsigned worker,uint32_t address){
 return PioneerQ38NextGdnOutputProgramWord(worker,address);
}
