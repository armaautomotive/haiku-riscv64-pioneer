/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FRONTEND_PROFILE_CLI_H
#define PIONEER_Q38_FRONTEND_PROFILE_CLI_H
#include <cstring>
enum class PioneerQ38FrontendProfileMode {Invalid,SourceOnly,Positions34};
static inline PioneerQ38FrontendProfileMode PioneerQ38FrontendProfileArguments(int argc,const char* const* argv){
 using M=PioneerQ38FrontendProfileMode;
 if(argc!=2 || !argv || !argv[0] || !argv[1])return M::Invalid;
 return !std::strcmp(argv[1],"--profile-frontends-34-positions")?M::Positions34:
  !std::strcmp(argv[1],"--preflight-source-only")?M::SourceOnly:M::Invalid;
}
#endif
