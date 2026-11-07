/* SPDX-License-Identifier: MIT */
#include "PioneerQ38FastDecodeProfileBuildRole.h"
#ifndef PIONEER_Q38_FAST_DECODE_PROFILE_SCOPE_H
#define PIONEER_Q38_FAST_DECODE_PROFILE_SCOPE_H
#include "PioneerQ38FastDecodeProfileData.h"
extern "C" unsigned q38_fast_decode_profile_enter(unsigned);
extern "C" void q38_token_phase_mark(unsigned);
class PioneerQ38FastDecodeProfileScope {
 unsigned previous;
public:
 explicit PioneerQ38FastDecodeProfileScope(unsigned phase):previous(q38_fast_decode_profile_enter(phase)){}
 void Next(unsigned phase){q38_token_phase_mark(phase);}
 ~PioneerQ38FastDecodeProfileScope(){if(previous<PioneerQ38FastDecodeProfileData::Phases)q38_token_phase_mark(previous);}
};
#endif
