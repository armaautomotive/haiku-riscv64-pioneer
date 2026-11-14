/* SPDX-License-Identifier: MIT */
// Private CPU-only preflight against an authenticated original, never device.
#if (defined(PIONEER_NATIVE_RECURRENT_BAND_SOURCE) + defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_SOURCE) \
 + defined(PIONEER_NATIVE_SMALL_ALPHA_BAND_SOURCE) + defined(PIONEER_NATIVE_SMALL_BETA_BAND_SOURCE) \
 + defined(PIONEER_NATIVE_RECURRENT_QKV_BAND_SOURCE)) != 1 \
 || (defined(PIONEER_NATIVE_RECURRENT_BAND_SOURCE) && PIONEER_NATIVE_RECURRENT_BAND_SOURCE != 1) \
 || (defined(PIONEER_NATIVE_RECURRENT_OUT_BAND_SOURCE) && PIONEER_NATIVE_RECURRENT_OUT_BAND_SOURCE != 1) \
 || (defined(PIONEER_NATIVE_SMALL_ALPHA_BAND_SOURCE) && PIONEER_NATIVE_SMALL_ALPHA_BAND_SOURCE != 1) \
 || (defined(PIONEER_NATIVE_SMALL_BETA_BAND_SOURCE) && PIONEER_NATIVE_SMALL_BETA_BAND_SOURCE != 1) \
 || (defined(PIONEER_NATIVE_RECURRENT_QKV_BAND_SOURCE) && PIONEER_NATIVE_RECURRENT_QKV_BAND_SOURCE != 1)
#error Recurrent source test requires its explicit private diagnostic role
#endif
#include "../PioneerNativeModelBandSource.h"
#include <cstdio>
#include <string>
int main(int argc,char** argv){
 if(argc!=2 || argv[1][0]!='/')return 2;
 PioneerNativeModelBandSource source;std::string error;
 if(!source.Prepare(argv[1],error) || !source.Ready()){
  std::fprintf(stderr,"RECURRENT_SOURCE_REFUSED %s; NO DEVICE\n",error.c_str());return 1;
 }
 std::puts("PASS recurrent source authenticates original and rederives all three private samples; NO DEVICE");
 return 0;
}
