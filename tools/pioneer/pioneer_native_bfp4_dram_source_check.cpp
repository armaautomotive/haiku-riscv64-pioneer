/* SPDX-License-Identifier: MIT */
// CPU-only, read-only authentication of the exact original and three fixed
// BFP4 sample/reference pairs. Never opens the P100A or creates model caches.
#include "PioneerNativeBfp4DramModelSource.h"
#include <cstdio>
#include <cstring>
int main(int argc,char** argv){
 if(argc!=2 || strcmp(argv[1],"--check-original-source-only")){
  fputs("Use --check-original-source-only; CPU read-only, no device.\n",stderr);return 2;
 }
 PioneerNativeBfp4DramModelSource source;std::string error;
 puts("BFP4_SOURCE_CHECK: authenticating original and regenerating fixed samples; NO DEVICE");
 if(fflush(stdout) || !source.Prepare("/boot/home/models/Qwen3.8-27B-Q4_K_M.gguf",error) || !source.Ready()){
  fprintf(stderr,"BFP4_SOURCE_CHECK_REFUSED: %s\n",error.c_str());return 1;
 }
 puts("PASS_BFP4_SOURCE_CHECK: authenticated original and three exact fixed sample/reference pairs; NO DEVICE");
 return fflush(stdout)?1:0;
}
