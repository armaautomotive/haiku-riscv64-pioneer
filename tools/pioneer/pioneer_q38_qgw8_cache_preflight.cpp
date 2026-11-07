/* SPDX-License-Identifier: MIT */
// Read-only CPU preflight. No device, ownership, upload, inference or Work.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "PioneerQ38HybridHeadVerifiedModel.h"

int main(int argc,char** argv){
 if(argc!=5 || strcmp(argv[1],"--verify-qgw8") || argv[2][0]!='/' || argv[3][0]!='/' || argv[4][0]!='/'){
  fputs("Use --verify-qgw8 ABSOLUTE_FFN_BFP4_CACHE ABSOLUTE_FFN_BFP8_CACHE ABSOLUTE_HEAD_BFP8_CACHE; CPU-only.\n",stderr);return 2;
 }
 auto model=std::make_unique<PioneerQ38HybridHeadVerifiedModel>();std::string error;
 puts("QGW8_PREFLIGHT authenticating original; CPU-only, no accelerator");fflush(stdout);
 if(!model->Prepare(argv[2],argv[3],argv[4],error,[](unsigned done,unsigned total){if(done%21==0 || done==total){
  printf("QGW8_PREFLIGHT verified %u/%u converted fragments\n",done,total);fflush(stdout);
 }}) || !model->Ready()){
  fprintf(stderr,"QGW8_PREFLIGHT_REFUSED: %s\n",error.c_str());return 1;
 }
 puts("PASS_QGW8_PREFLIGHT 1113 BFP4 FFN, 231 BFP8 FFN, 7 BFP8 head fragments; NO_DEVICE_UPLOAD_MODEL_EXECUTION_OR_SPEED");
 return fflush(stdout)?1:0;
}
