/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_NATIVE_ROOT_PROFILE_BRIDGE_TEST) || PIONEER_Q38_NATIVE_ROOT_PROFILE_BRIDGE_TEST != 1
#error Explicit isolated HOST instruction-profile bridge required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) || defined(__FAST_MATH__)
#error Native ROOT instruction-profile bridge is HOST-only,not kernel,CPU oracle,device code or Work
#endif
#include "pioneer_q38_native_root_profile_bridge.h"
#include "PioneerQ38InstructionProfile.h"
#include <cstdlib>
namespace {
const char* Map(){
 const char* path=std::getenv("PIONEER_TEST_ROOT_PROFILE_MAP");assert(path && *path);return path;
}
PioneerQ38InstructionProfile& Profile(){
 static PioneerQ38InstructionProfile profile(Map());return profile;
} // one continuous observation history;no reset API or hardware inputs
}
void Q38NativeRootProfileRetired(uint32_t pc){Profile().Record(pc);}
void Q38NativeRootProfileReport(uint64_t retired){Profile().Dump(retired);}
