/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_MIXED_LIVE_COMPARISON_H
#define PIONEER_Q38_MIXED_LIVE_COMPARISON_H
#include <stdint.h>
enum PioneerQ38MixedLiveReferenceStage : uint32_t {
 PioneerQ38MixedEmbedding,PioneerQ38MixedNormalized,PioneerQ38MixedQg,PioneerQ38MixedKey,PioneerQ38MixedValue,
 PioneerQ38MixedQuery,PioneerQ38MixedGate,PioneerQ38MixedHalfKey,PioneerQ38MixedHalfValue,PioneerQ38MixedAttention,
 PioneerQ38MixedProjection,PioneerQ38MixedResidual,PioneerQ38MixedPostNormalized,PioneerQ38MixedFfnGate,
 PioneerQ38MixedFfnUp,PioneerQ38MixedActivated,PioneerQ38MixedDown,PioneerQ38MixedFinal,PioneerQ38MixedStageCount
}; // Identifiers only; NO expected vectors or numerical/execution permissions.
struct PioneerQ38MixedLiveReferenceComparison {
 uint32_t checked=0,nonfinite=0,differences=0,passed=0;
 double maxAbsolute=0,relativeL2=0;
}; // CPU report DATA only; not execution, token, initialization or END permission
#endif
