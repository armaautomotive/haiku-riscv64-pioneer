/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RECURRENT_BLOCK_COMPARISON_H
#define PIONEER_Q38_RECURRENT_BLOCK_COMPARISON_H
#include <stdint.h>
// Identifiers/reports and post-END output boundary ONLY;NO CPU products,
// expected input,result,source initialization or native cleanup authority.
enum PioneerRecurrentOriginalStage : uint32_t {
 PioneerRecurrentEmbedding,PioneerRecurrentNormalized,PioneerRecurrentAlpha,PioneerRecurrentBeta,
 PioneerRecurrentZ,PioneerRecurrentQkv,PioneerRecurrentConv,PioneerRecurrentState,PioneerRecurrentRaw,
 PioneerRecurrentGated,PioneerRecurrentProjection,PioneerRecurrentResidual,PioneerRecurrentPostNormalized,
 PioneerRecurrentFfnGate,PioneerRecurrentFfnUp,PioneerRecurrentActivated,PioneerRecurrentDown,PioneerRecurrentFinal,
 PioneerRecurrentStageCount
};
struct PioneerRecurrentOriginalComparison {
 uint32_t checked=0,differences=0,nonfinite=0,passed=0;
 double maxAbsolute=0,relativeL2=0;
};
struct RecurrentOriginalStageSink {
 virtual ~RecurrentOriginalStageSink()=default;
 virtual bool Publish(PioneerRecurrentOriginalStage,const uint32_t*,unsigned)=0;
};
#endif
