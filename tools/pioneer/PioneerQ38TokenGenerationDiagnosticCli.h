/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_GENERATION_DIAGNOSTIC_CLI_H
#define PIONEER_Q38_TOKEN_GENERATION_DIAGNOSTIC_CLI_H
#include <cstring>
enum class PioneerQ38TokenGenerationDiagnosticMode {Invalid,SourceOnly,ThreePositions};
static inline PioneerQ38TokenGenerationDiagnosticMode
PioneerQ38TokenGenerationDiagnosticArguments(int argc,const char* const* argv){
 if(argc!=2 || !argv || !argv[1])return PioneerQ38TokenGenerationDiagnosticMode::Invalid;
 if(!std::strcmp(argv[1],"--preflight-source-only"))return PioneerQ38TokenGenerationDiagnosticMode::SourceOnly;
 if(!std::strcmp(argv[1],"--generate-three-positions"))return PioneerQ38TokenGenerationDiagnosticMode::ThreePositions;
 return PioneerQ38TokenGenerationDiagnosticMode::Invalid;
}
#endif
