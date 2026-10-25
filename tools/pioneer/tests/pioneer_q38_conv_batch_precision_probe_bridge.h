/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CONV_BATCH_PRECISION_PROBE_BRIDGE_H
#define PIONEER_Q38_CONV_BATCH_PRECISION_PROBE_BRIDGE_H
#include <cstdint>
struct PioneerQ38ConvPrecisionProbeStats {unsigned workers,groups,maxStack,maxSteps;};
PioneerQ38ConvPrecisionProbeStats ExecuteConvBatchPrecisionProbe(const char* image,const uint32_t* input,
 const uint32_t* weights,uint32_t* output,uint32_t* next);
#endif
