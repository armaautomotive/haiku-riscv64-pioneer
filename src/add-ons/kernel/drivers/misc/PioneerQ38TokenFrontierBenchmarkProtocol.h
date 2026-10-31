/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_PROTOCOL_H
#define PIONEER_Q38_TOKEN_FRONTIER_BENCHMARK_PROTOCOL_H
#include "PioneerQ38TokenDecodeBenchmarkProtocol.h"
#include "PioneerQ38TokenFrontierPacket.h"
// Same fixed2prompt/34position DATA shape;distinct actual control route.
#define TT_PCI_PROBE_Q38_TOKEN_FRONTIER_BENCHMARK (B_DEVICE_OP_CODES_END+0x6607)
namespace PioneerQ38TokenFrontierBenchmark {
static constexpr uint64_t BuildTag=PioneerQ38TokenDecodeBenchmark::BuildTag;
static inline bool ControlRequestShape(const PioneerQ38TokenControl::Packet& p){
 return PioneerQ38TokenDecodeBenchmark::ControlRequestShape(p);
}
}
#endif
