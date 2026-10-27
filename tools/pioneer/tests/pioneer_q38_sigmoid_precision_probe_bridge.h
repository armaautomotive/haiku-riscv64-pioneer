/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_SIGMOID_PRECISION_PROBE_BRIDGE_TEST_H
#define PIONEER_Q38_SIGMOID_PRECISION_PROBE_BRIDGE_TEST_H
#include <stdint.h>
// Isolated LOCAL arithmetic VM inputs ONLY. This cannot grant a genuine
// on-card producer, original-model/native context or device permission.
void ExecuteSigmoidPrecisionProbe(const char*,const uint32_t*,uint32_t*);
#endif
