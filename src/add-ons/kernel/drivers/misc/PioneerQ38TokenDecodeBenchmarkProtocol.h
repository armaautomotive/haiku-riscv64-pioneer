/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_TOKEN_DECODE_BENCHMARK_PROTOCOL_H
#define PIONEER_Q38_TOKEN_DECODE_BENCHMARK_PROTOCOL_H
#include "PioneerQ38TokenHiddenValidationProtocol.h"
// Separate bounded, no-CPU-reference diagnostic. Not the short tag75
// endpoint, arbitrary prompts, EOS coverage, streaming, or Assistant Work.
#define TT_PCI_PROBE_Q38_TOKEN_DECODE_BENCHMARK (B_DEVICE_OP_CODES_END+0x6606)
namespace PioneerQ38TokenDecodeBenchmark {
static constexpr uint64_t BuildTag=PioneerQ38TokenHiddenValidation::BuildTag;
enum {Prompts=2,Positions=34,FirstToken=248319,SecondToken=17,
 FeedbackPositions=Positions-Prompts};
static_assert(unsigned(Positions)<=unsigned(PioneerQ38TokenControl::Batch),"one bounded post-HOLD drain");
static inline bool ControlRequestShape(const PioneerQ38TokenControl::Packet& p){
 namespace P=PioneerQ38TokenControl;
 if(!P::RequestShape(p))return false;
 const auto& r=p.request;
 if(r.operation==P::Start)return r.prompts==Prompts && r.limit==Positions && !r.stopEos
  && !r.eos0 && !r.eos1 && p.prompt[0]==FirstToken && p.prompt[1]==SecondToken;
 // No cancellation/capture or partial-record drain in this normal-run
 // diagnostic. The native owner still proves terminal/HOLD/all records/END.
 return r.operation!=P::Cancel && (r.operation!=P::Read || r.records==Positions);
}
}
#endif
