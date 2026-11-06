/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Bf16ProfileBuildRole.h"
#ifndef PIONEER_Q38_BF16_PROFILE_SCOPE_H
#define PIONEER_Q38_BF16_PROFILE_SCOPE_H
#include "PioneerQ38Bf16ProfileData.h"
extern "C" unsigned q38_bf16_profile_enter(unsigned);
extern "C" void q38_token_phase_mark(unsigned);
// Only timing DATA changes. No caller-supplied completed phase or products.
class PioneerQ38Bf16ProfileScope {
 unsigned previous;
public:
 explicit PioneerQ38Bf16ProfileScope(unsigned phase):previous(q38_bf16_profile_enter(phase)){}
 void Next(unsigned phase){q38_token_phase_mark(phase);}
 ~PioneerQ38Bf16ProfileScope(){if(previous<PioneerQ38Bf16ProfileData::Phases)q38_token_phase_mark(previous);}
};
#endif
