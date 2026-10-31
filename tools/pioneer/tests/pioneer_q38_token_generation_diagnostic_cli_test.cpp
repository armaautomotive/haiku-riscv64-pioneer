/* SPDX-License-Identifier: MIT */
#include "../PioneerQ38TokenGenerationDiagnosticCli.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
 using M=PioneerQ38TokenGenerationDiagnosticMode;
 const char* good[]={"diagnostic","--generate-three-positions",nullptr};
 const char* source[]={"diagnostic","--preflight-source-only",nullptr};
 const char* missing[]={"diagnostic",nullptr};
 assert(PioneerQ38TokenGenerationDiagnosticArguments(2,good)==M::ThreePositions);
 assert(PioneerQ38TokenGenerationDiagnosticArguments(2,source)==M::SourceOnly);
 unsigned refused=0;
 for(int count:{-1,0,1,3,4,999}){
  assert(PioneerQ38TokenGenerationDiagnosticArguments(count,good)==M::Invalid);refused++;
 }
 assert(PioneerQ38TokenGenerationDiagnosticArguments(2,nullptr)==M::Invalid);refused++;
 assert(PioneerQ38TokenGenerationDiagnosticArguments(2,missing)==M::Invalid);refused++;
 for(const char* text:{"","--help","--work","--cpu","--validate-three-positions","--validate-cancel-boundary",
                      "--capture-first-layer","--capture-layer-boundary","--generate-three-positions=1",
                      "--generate-three-positions "," --generate-three-positions","--Generate-three-positions",
                      "--generate-three-position","--preflight-source-only=1","--preflight-source-only "}){
  const char* args[]={"diagnostic",text,nullptr};
  assert(PioneerQ38TokenGenerationDiagnosticArguments(2,args)==M::Invalid);refused++;
 }
 printf("PASS fixed generation diagnostic CLI/2 positive/%u malformed+old-mode+Work+CPU+cancel+capture refusals;pure parser only,NO source/device/native/physical/TPS proof\n",refused);
}
