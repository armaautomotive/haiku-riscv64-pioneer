/* SPDX-License-Identifier: MIT */
// Actual conventional driver/context/IO/control/closed routing,UNLINKED.
// Distinct DEBUG68,not a renamed/loadable variant of compile-only67.
#define PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE 1
#define PIONEER_Q38_RESIDENT_OUTPUT_OWNER 1
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_PROJECTION_RESULT_STASH 1
#define PIONEER_Q38_PROJECTION_EXECUTION_OWNER 1
#define PIONEER_Q38_LEARNED_RMS_NATIVE 1
#define PIONEER_Q38_EMBEDDING_NATIVE 1
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
extern "C" status_t pioneer_output_debug_control(PioneerProbeCookie* c,uint32 operation,void* data,size_t length){
 return Q38ResidentOutputValidationControl(c,operation,data,length);
}
extern "C" status_t pioneer_output_debug_route(void* c,uint32 operation,void* data,size_t length){
 return ProbeControl(c,operation,data,length);
}
