/* SPDX-License-Identifier: MIT */
// Read-only, CPU-only preflight of the actual proposed hybrid pages. This is
// not a device owner, an uploader, model inference, or Work/Assistant mode.
#if !defined(PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC) || PIONEER_Q38_BFP4_PREPARATION_DIAGNOSTIC != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) \
 || defined(PIONEER_Q38_CPU_VALIDATION) || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Hybrid cache preflight requires its distinct CPU-only preparation role
#endif
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "PioneerQ38HybridVerifiedModel.h"

int main(int argc,char** argv){
 if(argc!=4 || strcmp(argv[1],"--verify-prepared") || argv[2][0]!='/' || argv[3][0]!='/'){
  fputs("Use --verify-prepared ABSOLUTE_BFP4_CACHE ABSOLUTE_BFP8_CACHE; CPU-only.\n",stderr);return 2;
 }
 auto model=std::make_unique<PioneerQ38HybridVerifiedModel>();std::string error;
 puts("HYBRID_PREFLIGHT authenticating original; CPU-only, no accelerator");fflush(stdout);
 if(!model->Prepare(argv[2],argv[3],error,[](unsigned done,unsigned total){if(done%21==0 || done==total){
  printf("HYBRID_PREFLIGHT verified %u/%u converted fragments\n",done,total);fflush(stdout);
 }}) || !model->Ready()){
  fprintf(stderr,"HYBRID_PREFLIGHT_REFUSED: %s\n",error.c_str());return 1;
 }
 puts("PASS_HYBRID_PREFLIGHT 1071 BFP4 plus 273 BFP8 fragments, existing caches verified; NO_DEVICE_UPLOAD_MODEL_EXECUTION_OR_SPEED");
 return fflush(stdout)?1:0;
}
