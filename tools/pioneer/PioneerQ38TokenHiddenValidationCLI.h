/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLI_H
#define PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLI_H
#include <cstring>
// DATA grammar only;no files,model source,device or driver activation.
struct PioneerQ38TokenHiddenValidationOptions {bool preflight=false;const char* output=nullptr;};
static inline bool PioneerQ38TokenHiddenValidationArguments(int argc,const char* const* argv,
 PioneerQ38TokenHiddenValidationOptions& output){
 if(!argv || argc<2 || !argv[1])return false;
 PioneerQ38TokenHiddenValidationOptions selected;
 if(argc==2 && !std::strcmp(argv[1],"--preflight-source-only"))selected.preflight=true;
 else if(argc==3 && !std::strcmp(argv[1],"--capture-first-layer") && argv[2]){
  const size_t n=std::strlen(argv[2]);
  if(n<2 || argv[2][0]!='/' || argv[2][n-1]=='/')return false;
  selected.output=argv[2];
 }else return false;
 output=selected;return true;
}
#endif
