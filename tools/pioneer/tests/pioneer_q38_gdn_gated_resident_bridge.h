/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GDN_GATED_RESIDENT_BRIDGE_H
#define PIONEER_Q38_GDN_GATED_RESIDENT_BRIDGE_H
#include "pioneer_q38_decoder_conv_bridge.h"
// TEST original-byte endpoint only. This separate execution is NOT hooked
// into the native owner/Work path. No computed raw/gated reference enters.
struct GatedResidentObserved {
 uint32_t prepared[2][322],next[2][8192],raw[2][64],gated[2][64];
};
void ExecuteGdnGatedResidentOriginalBytes(const char*,const PioneerQ38CanonicalModelPlan&,unsigned bank,
 const uint32_t input[2][64],const uint32_t state[2][64],const uint32_t norm[128],
 const std::vector<uint32_t>& workspace,ProjectionExecutionLearnedBytes&,DecoderHistoryBytes&,GatedResidentObserved&);
#endif
