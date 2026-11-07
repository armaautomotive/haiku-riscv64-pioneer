/* SPDX-License-Identifier: MIT */
#if defined(PIONEER_Q38_FULL_OFFLOAD_PROTOCOL_H) || defined(PIONEER_Q38_BF16_FULL_PROTOCOL_H)
#error Hybrid full-card protocol cannot alias an older driver protocol
#endif
#ifndef PIONEER_Q38_HYBRID_FULL_PROTOCOL_H
#define PIONEER_Q38_HYBRID_FULL_PROTOCOL_H
#include "PioneerQ38TokenControlPacket.h"
#include "../../../../../tools/pioneer/PioneerQ38FullOffloadEog.h"
#define TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT (B_DEVICE_OP_CODES_END+0x8800)
#define TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL (B_DEVICE_OP_CODES_END+0x8801)
namespace PioneerQ38FullOffload {
static constexpr uint64_t BuildTag=UINT64_C(0x2026110700010125);
enum {Version=1,MaxPrompt=96,MaxOutput=32,MaxPositions=128,Vocabulary=248320};
enum {Matrices=1,Attention=2,Recurrent=4,KV=8,Embedding=16,Normalize=32,
 Residual=64,Activation=128,Greedy=256,AllCompute=511};
struct Support {uint32_t driver,version,prompt,output,positions,vocabulary,compute,reserved;};
static_assert(sizeof(Support)==32,"pointer-free complete-model capability DATA");
static inline bool SupportRequest(const Support& s){
 return !s.driver && !s.version && !s.prompt && !s.output && !s.positions && !s.vocabulary && !s.compute && !s.reserved;
}
static inline bool Supported(const Support& s){
 return s.driver==55 && s.version==Version && s.prompt==MaxPrompt && s.output==MaxOutput
  && s.positions==MaxPositions && s.vocabulary==Vocabulary && s.compute==AllCompute && !s.reserved;
}
static inline bool RequestShape(const PioneerQ38TokenControl::Packet& p){
 namespace P=PioneerQ38TokenControl;if(!P::RequestShape(p))return false;const auto& r=p.request;
 if(r.operation==P::Start)return r.prompts<=MaxPrompt && r.limit<=MaxPositions && r.limit-r.prompts+1<=MaxOutput
  && r.stopEos==1 && r.eos0==PioneerQ38FullOffloadEog::Eos0 && r.eos1==PioneerQ38FullOffloadEog::Eos1;
 return true;
}
}
#endif
