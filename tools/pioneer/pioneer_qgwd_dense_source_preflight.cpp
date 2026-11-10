/* SPDX-License-Identifier: MIT */
// Real-Qwen authenticated diagnostic source only; NEVER opens the card.
#if !defined(PIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE) || PIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGWD dense source preflight is isolated CPU-only diagnostic
#endif
#include "PioneerNativeModelBandSource.h"
#include <cstdio>
#include <string>
int main(int argc,char**){
 if(argc!=1)return 2;
 std::puts("AUTHENTICATING_QGWD_DENSE_SOURCE; NO DEVICE");std::fflush(stdout);
 PioneerNativeModelBandSource source;std::string error;
 if(!source.Prepare("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",error) || !source.Ready()){
  std::fprintf(stderr,"DENSE_SOURCE_PREFLIGHT_REFUSED %s; NO DEVICE\n",error.c_str());return 1;
 }
 std::puts("PASS_QGWD_DENSE_SOURCE tensor45 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED");
 return std::fflush(stdout)?1:0;
}
