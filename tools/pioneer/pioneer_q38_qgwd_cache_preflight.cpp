/* SPDX-License-Identifier: MIT */
// Read-only four-cache preflight. No device, owner, upload, inference or Work.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "PioneerQ38HybridDenseQ8VerifiedModel.h"

int main(int argc,char** argv){
 if(argc!=6 || std::strcmp(argv[1],"--verify-qgwd") || argv[2][0]!='/' || argv[3][0]!='/' || argv[4][0]!='/' || argv[5][0]!='/'){
  std::fputs("Use --verify-qgwd ABSOLUTE_FFN_BFP4 ABSOLUTE_FFN_BFP8 ABSOLUTE_HEAD_BFP8 ABSOLUTE_DENSE_BFP8; CPU-only.\n",stderr);return 2;
 }
 auto model=std::make_unique<PioneerQ38HybridDenseQ8VerifiedModel>();std::string error;
 std::puts("QGWD_PREFLIGHT authenticating one original and four caches; CPU-only, no accelerator");std::fflush(stdout);
 if(!model->Prepare(argv[2],argv[3],argv[4],argv[5],error,[](unsigned done,unsigned total){if(done%21==0 || done==total){
  std::printf("QGWD_PREFLIGHT parent fragments %u/%u\n",done,total);std::fflush(stdout);
 }}) || !model->Ready() || !model->PreparedPages() || !model->Canonical()){
  std::fprintf(stderr,"QGWD_PREFLIGHT_REFUSED: %s\n",error.c_str());return 1;
 }
 std::puts("PASS_QGWD_PREFLIGHT 1113 BFP4 FFN, 231 BFP8 FFN, 7 BFP8 head, 336 BFP8 dense fragments; NO_DEVICE_UPLOAD_MODEL_EXECUTION_OR_SPEED");
 return std::fflush(stdout)?1:0;
}
