/* SPDX-License-Identifier: MIT */
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputReadbackPlan.h"
extern "C" bool pioneer_output_readback_build_data(PioneerQ38ResidentOutputReadbackPlan& heap,
 const PioneerQ38CanonicalModelPlan& canonical,const PioneerQ38ResidentOutputPlan& operation,
 unsigned layer,unsigned position,unsigned bank,unsigned slot,unsigned token,unsigned phase,
 const PioneerQ38GdnIdentity& id,const uint32_t* sources,const uint32_t* sequence){
 return heap.Build(canonical,operation,layer,position,bank,slot,token,phase,id,sources,sequence);
}
extern "C" bool pioneer_output_readback_page_data(const PioneerQ38ResidentOutputReadbackPlan& heap,
 unsigned ordinal,PioneerQ38ResidentOutputReadbackPage& page){return heap.Page(ordinal,page);}
