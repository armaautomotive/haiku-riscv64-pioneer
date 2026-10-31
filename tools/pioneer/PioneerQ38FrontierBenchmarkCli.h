/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FRONTIER_BENCHMARK_CLI_H
#define PIONEER_Q38_FRONTIER_BENCHMARK_CLI_H
#include <cstring>
enum class PioneerQ38FrontierBenchmarkMode {Invalid,SourceOnly,Positions34};
static inline PioneerQ38FrontierBenchmarkMode PioneerQ38FrontierBenchmarkArguments(int argc,const char* const* argv){
 using M=PioneerQ38FrontierBenchmarkMode;
 if(argc!=2 || !argv || !argv[0] || !argv[1])return M::Invalid;
 return !std::strcmp(argv[1],"--benchmark-34-positions")?M::Positions34:
  !std::strcmp(argv[1],"--preflight-source-only")?M::SourceOnly:M::Invalid;
}
#endif
