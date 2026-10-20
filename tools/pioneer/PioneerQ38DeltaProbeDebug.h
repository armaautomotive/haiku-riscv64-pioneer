/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DELTA_PROBE_DEBUG_H
#define PIONEER_Q38_DELTA_PROBE_DEBUG_H
#include <math.h>
#include <string.h>
#include "PioneerAttentionMath.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38DeltaProbeProtocol.h"
// EXPLICIT CPU-reference debug fixture,never a production Work fallback.
// Q is pre-scaled. This qualifies the vector primitive,not fused GGML GDN
// rounding (which scales after the dot) or resident model-state routing.
static inline void PioneerQ38DeltaDebugFixture(tt_q38_delta_probe& r,unsigned trial)
{
 memset(&r,0,sizeof(r)); r.version=TT_Q38_DELTA_PROBE_VERSION;
 r.decay=PioneerAttentionBits(trial==2?0:trial==3?1:.83f);
 r.beta=PioneerAttentionBits(trial==2?1:trial==3?0:.37f);
 for (unsigned i=0;i<8192;i++) r.prior[i]=PioneerAttentionBits(trial?float(sin(i*.031+trial*.17)*.003):0);
 for (unsigned i=0;i<128;i++) {
  r.query[i]=PioneerAttentionBits(trial?float(sin(i*.123+trial*.317)*.01):0);
  r.key[i]=PioneerAttentionBits(trial?float(cos(i*.153+trial*.217)*.03):0);
 }
 for (unsigned i=0;i<64;i++) r.value[i]=PioneerAttentionBits(trial?float(sin(i*.013+trial*.117)*2.1):0);
}
static inline unsigned PioneerQ38DeltaDebugErrors(const tt_q38_delta_probe& r)
{
 unsigned errors=0;
 float decay=PioneerAttentionFloat(r.decay),beta=PioneerAttentionFloat(r.beta);
 for (unsigned j=0;j<64;j++) {
  float prediction=0,output=0;
  for (unsigned i=0;i<128;i++) prediction+=(PioneerAttentionFloat(r.prior[i*64+j])*decay)*PioneerAttentionFloat(r.key[i]);
  float delta=(PioneerAttentionFloat(r.value[j])-prediction)*beta;
  for (unsigned i=0;i<128;i++) {
   float next=PioneerAttentionFloat(r.prior[i*64+j])*decay+PioneerAttentionFloat(r.key[i])*delta;
   errors+=r.next[i*64+j]!=PioneerAttentionBits(next);
   output+=next*PioneerAttentionFloat(r.query[i]);
  }
  errors+=r.output[j]!=PioneerAttentionBits(output);
 }
 return errors;
}
#endif
