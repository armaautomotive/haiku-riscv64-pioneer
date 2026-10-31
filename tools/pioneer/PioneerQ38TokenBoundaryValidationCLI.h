/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLI_H
#define PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLI_H
#include <cstring>
struct PioneerQ38TokenBoundaryValidationOptions {bool preflight=false;unsigned delayMs=0;const char* output=nullptr;};
static inline bool PioneerQ38TokenBoundaryValidationArguments(int argc,const char* const* argv,
 PioneerQ38TokenBoundaryValidationOptions& output){
 if(!argv || argc<2 || !argv[1])return false;
 PioneerQ38TokenBoundaryValidationOptions selected;
 if(argc==2 && !std::strcmp(argv[1],"--preflight-source-only"))selected.preflight=true;
 else if(argc==4 && !std::strcmp(argv[1],"--capture-layer-boundary") && argv[2] && argv[3]){
  const size_t n=std::strlen(argv[3]);
  if(n<2 || argv[3][0]!='/' || argv[3][n-1]=='/' || !argv[2][0])return false;
  // Canonical bounded decimal ONLY;no signs,whitespace,overflow or units.
  if(argv[2][0]=='0' && argv[2][1])return false;
  for(const char* p=argv[2];*p;p++){
   if(*p<'0' || *p>'9' || selected.delayMs>500)return false;
   selected.delayMs=selected.delayMs*10+unsigned(*p-'0');
   if(selected.delayMs>5000)return false;
  }
  selected.output=argv[3];
 }else return false;
 output=selected;return true;
} // grammar only;no file/source/device access or exact-layer selection
#endif
