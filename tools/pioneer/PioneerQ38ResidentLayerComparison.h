/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RESIDENT_LAYER_COMPARISON_H
#define PIONEER_Q38_RESIDENT_LAYER_COMPARISON_H
#include <stdint.h>
// CPU DEBUG observation DATA only,not a device ABI, pointer/route request,
// ready bit, source/native permission or Work-mode numerical fallback.
struct PioneerQ38ResidentLayerComparison {
 enum {Words=1630208,Final=0,Conv=5120,History=15360,Gated=46080,Projection=52224,Current=57344,Next=843776};
 uint32_t version=1,checked=0,nonfinite=0,convErrors=0,historyErrors=0,gatedErrors=0;
 uint32_t projectionDifferences=0,currentErrors=0,nextErrors=0,finalDifferences=0,passed=0,reserved=0;
 double projectionMaxAbsolute=0,finalMaxAbsolute=0,finalRelativeL2=0;
};
static_assert(sizeof(PioneerQ38ResidentLayerComparison)==72,"CPU observation fields,NO implicit padding");
static_assert(PioneerQ38ResidentLayerComparison::Next+96*8192==PioneerQ38ResidentLayerComparison::Words,"NEW QLV1 layout,NOT old partial-prefix payload");
#endif
