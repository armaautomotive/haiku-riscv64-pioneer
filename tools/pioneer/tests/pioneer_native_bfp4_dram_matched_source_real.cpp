/* SPDX-License-Identifier: MIT */
// CPU-only same-open authentication against the separately pinned matched
// samples and private metadata. Does not open an accelerator.
#if !defined(PIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC) || PIONEER_Q38_BFP4_MATCHED_DIAGNOSTIC != 1
#error Explicit matched-input diagnostic role required
#endif
#include "../PioneerNativeBfp4DramModelSource.h"
#include <cstdio>
int main(int argc,char** argv){
 if(argc!=2 || argv[1][0]!='/')return 2;
 PioneerNativeBfp4DramModelSource source;std::string error;
 puts("MATCHED_BFP4_SOURCE authenticating original and regenerating three samples; CPU_ONLY");fflush(stdout);
 if(!source.Prepare(argv[1],error) || !source.Ready()){
  fprintf(stderr,"MATCHED_BFP4_SOURCE_REFUSED: %s\n",error.c_str());return 1;
 }
 puts("PASS_MATCHED_BFP4_SOURCE original hash, three sample hashes and 96 metadata words exact; NO_DEVICE_OR_SPEED");
 return fflush(stdout)?1:0;
}
