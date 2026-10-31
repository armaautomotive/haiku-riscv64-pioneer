/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DECODE_BENCHMARK_CLI_H
#define PIONEER_Q38_DECODE_BENCHMARK_CLI_H
#include <cstring>
enum class PioneerQ38DecodeBenchmarkMode {Invalid,SourceOnly,Positions34};
static inline PioneerQ38DecodeBenchmarkMode PioneerQ38DecodeBenchmarkArguments(int argc,const char* const* argv){
 using M=PioneerQ38DecodeBenchmarkMode;
 if(argc!=2 || !argv || !argv[0] || !argv[1])return M::Invalid;
 return !std::strcmp(argv[1],"--generate-34-positions")?M::Positions34:
  !std::strcmp(argv[1],"--preflight-source-only")?M::SourceOnly:M::Invalid;
}
#endif
