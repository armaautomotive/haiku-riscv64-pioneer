/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "../PioneerQ38PublicTiming.h"
#define PIONEER_Q38_NATIVE_Q8_MODEL_QUALIFICATION
static int Guard(bool repeatedRun,unsigned limit,bool graph) {
#include "native_q8_model_argument_guard.inc"
 return 0;
}
int main() {
 unsigned accepted=0;
 for(bool repeat:{false,true})for(unsigned limit:{0U,1U,4U,31U,32U,33U})for(bool graph:{false,true}) {
  bool okay=repeat && limit==32 && !graph;
  assert(Guard(repeat,limit,graph)==(okay?0:2));accepted+=okay;
 }
 assert(accepted==1);
 for(const char* argument:{"--public-one-token","--public-four-tokens","--public-32-tokens",
   "--public-three-32-token-runs","--public-q8-graph-export","invalid"}) {
  unsigned limit=PioneerQ38PublicTokenLimit(argument),sequences=PioneerQ38PublicSequenceCount(argument);
  assert(Guard(sequences==3,limit,false)==(!strcmp(argument,"--public-three-32-token-runs")?0:2));
 }
 puts("PASS ACTUAL public-model SM7 argument guard:ONLY THREE32 public non-graph diagnostic;all24 repeat/limit/graph combinations;NO source/device IO");
}
