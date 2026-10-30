/* SPDX-License-Identifier: MIT */
// This TU is compiled separately WITHOUT the emulator's legacy math flags.
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE) \
 || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_ATTENTION_FREESTANDING) || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE)
#error EOS state serializer implementation is HOST DATA only,never native,math-image,CPU-oracle or Work
#endif
#define PIONEER_Q38_TEST_OBSERVED_TRACE 1
#include "PioneerQ38EosStateDataBridge.h"
#include "pioneer_q38_state_snapshot_data.h"
bool SaveQ38ObservedEosStateData(const char* directory,unsigned layer,unsigned kind,const uint32_t* words,unsigned count){
 return Q38StateSnapshotData::SaveEos(directory,true,layer,kind,words,count);
}
bool FinishQ38ObservedEosStateData(const char* directory,unsigned count){return Q38StateSnapshotData::FinishEos(directory,true,count);}
