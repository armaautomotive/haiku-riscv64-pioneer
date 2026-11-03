/* SPDX-License-Identifier: MIT */
#include "PioneerQ38TokenPhaseProfileClientRole.h"
static_assert(PioneerQ38TokenPhaseProfileClientSelection==1,"separate phase timing CLI only");
#ifndef PIONEER_Q38_TOKEN_PHASE_PROFILE_CLI_H
#define PIONEER_Q38_TOKEN_PHASE_PROFILE_CLI_H
#include <cstring>
enum class PioneerQ38TokenPhaseProfileMode {Invalid,SourceOnly,Positions34};
static inline PioneerQ38TokenPhaseProfileMode PioneerQ38TokenPhaseProfileArguments(int argc,const char* const* argv){
 using M=PioneerQ38TokenPhaseProfileMode;
 if(argc!=2 || !argv || !argv[0] || !argv[1])return M::Invalid;
#ifdef PIONEER_Q38_TOKEN_MANUAL_TTSYNC_CLIENT
 return !std::strcmp(argv[1],"--benchmark-manual-ttsync-34-positions")?M::Positions34:
#elif defined(PIONEER_Q38_TOKEN_MATRIX_PHASE_CLIENT)
 return !std::strcmp(argv[1],"--profile-q4-jobs-34-positions")?M::Positions34:
#elif defined(PIONEER_Q38_TOKEN_CONSTANT_Q4_CLIENT)
 return !std::strcmp(argv[1],"--benchmark-constant-q4-34-positions")?M::Positions34:
#else
 return !std::strcmp(argv[1],"--profile-card-phases-34-positions")?M::Positions34:
#endif
  !std::strcmp(argv[1],"--preflight-source-only")?M::SourceOnly:M::Invalid;
}
#endif
