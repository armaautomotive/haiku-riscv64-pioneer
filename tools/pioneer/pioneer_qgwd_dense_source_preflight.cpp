/* SPDX-License-Identifier: MIT */
// Real-Qwen authenticated diagnostic source only; NEVER opens the card.
#if (defined(PIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE) + defined(PIONEER_NATIVE_RECURRENT_BAND_SOURCE) + defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_SOURCE) + defined(PIONEER_NATIVE_SMALL_ALPHA_BAND_SOURCE) + defined(PIONEER_NATIVE_SMALL_BETA_BAND_SOURCE) + defined(PIONEER_NATIVE_RECURRENT_QKV_BAND_SOURCE)) != 1 \
 || (defined(PIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE) && PIONEER_NATIVE_QGWD_DENSE_BAND_SOURCE != 1) \
 || (defined(PIONEER_NATIVE_RECURRENT_BAND_SOURCE) && PIONEER_NATIVE_RECURRENT_BAND_SOURCE != 1) \
 || (defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_SOURCE) && PIONEER_NATIVE_RECURRENT_OUT_BAND_SOURCE != 1) \
 || (defined(PIONEER_NATIVE_SMALL_ALPHA_BAND_SOURCE) && PIONEER_NATIVE_SMALL_ALPHA_BAND_SOURCE != 1) \
 || (defined(PIONEER_NATIVE_SMALL_BETA_BAND_SOURCE) && PIONEER_NATIVE_SMALL_BETA_BAND_SOURCE != 1) \
 || (defined(PIONEER_NATIVE_RECURRENT_QKV_BAND_SOURCE) && PIONEER_NATIVE_RECURRENT_QKV_BAND_SOURCE != 1) \
 || defined(_KERNEL_MODE) || defined(PIONEER_DRIVER_BUILD_TAG) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error QGWD dense source preflight is isolated CPU-only diagnostic
#endif
#include "PioneerNativeModelBandSource.h"
#include <cstdio>
#include <string>
int main(int argc,char**){
 if(argc!=1)return 2;
#if defined(PIONEER_NATIVE_RECURRENT_QKV_BAND_SOURCE)
 std::puts("AUTHENTICATING_RECURRENT_QKV_SOURCE; NO DEVICE");std::fflush(stdout);
#elif defined(PIONEER_NATIVE_SMALL_ALPHA_BAND_SOURCE) || defined(PIONEER_NATIVE_SMALL_BETA_BAND_SOURCE)
 std::puts("AUTHENTICATING_SMALL_RECURRENT_SOURCE; NO DEVICE");std::fflush(stdout);
#elif defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_SOURCE)
 std::puts("AUTHENTICATING_RECURRENT_OUT_SOURCE; NO DEVICE");std::fflush(stdout);
#elif defined(PIONEER_NATIVE_RECURRENT_BAND_SOURCE)
 std::puts("AUTHENTICATING_RECURRENT_SOURCE; NO DEVICE");std::fflush(stdout);
#else
 std::puts("AUTHENTICATING_QGWD_DENSE_SOURCE; NO DEVICE");std::fflush(stdout);
#endif
 PioneerNativeModelBandSource source;std::string error;
 if(!source.Prepare("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",error) || !source.Ready()){
  std::fprintf(stderr,"DENSE_SOURCE_PREFLIGHT_REFUSED %s; NO DEVICE\n",error.c_str());return 1;
 }
#if defined(PIONEER_NATIVE_RECURRENT_QKV_BAND_SOURCE)
 std::puts("PASS_RECURRENT_QKV_SOURCE tensor5 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED");
#elif defined(PIONEER_NATIVE_SMALL_ALPHA_BAND_SOURCE)
 std::puts("PASS_SMALL_ALPHA_SOURCE tensor11 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED");
#elif defined(PIONEER_NATIVE_SMALL_BETA_BAND_SOURCE)
 std::puts("PASS_SMALL_BETA_SOURCE tensor12 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED");
#elif defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_SOURCE)
 std::puts("PASS_RECURRENT_OUT_SOURCE tensor16 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED");
#elif defined(PIONEER_NATIVE_RECURRENT_BAND_SOURCE)
 std::puts("PASS_RECURRENT_SOURCE tensor3 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED");
#else
 std::puts("PASS_QGWD_DENSE_SOURCE tensor45 samples3 fresh_payload_and_reference_exact; NO DEVICE OR SPEED");
#endif
 return std::fflush(stdout)?1:0;
}
