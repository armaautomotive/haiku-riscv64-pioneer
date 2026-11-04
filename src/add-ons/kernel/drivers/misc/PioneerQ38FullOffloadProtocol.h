/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FULL_OFFLOAD_PROTOCOL_H
#define PIONEER_Q38_FULL_OFFLOAD_PROTOCOL_H
#include "PioneerQ38TokenControlPacket.h"
#include "../../../../../tools/pioneer/PioneerQ38FullOffloadEog.h"
// Separate from the fixed diagnostic endpoints and from hybrid ggml Work.
// Only prompt IDs/limits enter. No host hidden states, logits, KV, attention,
// learned products, addresses, firmware, or completion hints are accepted.
#define TT_PCI_PROBE_Q38_FULL_OFFLOAD_SUPPORT (B_DEVICE_OP_CODES_END+0x8800)
#define TT_PCI_PROBE_Q38_FULL_OFFLOAD_CONTROL (B_DEVICE_OP_CODES_END+0x8801)
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
#include "PioneerQ38TokenFrontierPacket.h" // pointer-free DATA only,not its old ioctl/diagnostic role
#define TT_PCI_PROBE_Q38_FULL_OFFLOAD_COUNTER (B_DEVICE_OP_CODES_END+0x8802)
#endif
namespace PioneerQ38FullOffload {
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
#if PIONEER_Q38_FULL_OFFLOAD_METRICS != 1 || !defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED) || PIONEER_Q38_FULL_OFFLOAD_BATCHED != 1
#error Full offload metrics requires exact batched selection
#endif
static constexpr uint64_t BuildTag=UINT64_C(0x2026110400010090);
#elif defined(PIONEER_Q38_FULL_OFFLOAD_BATCHED)
#if PIONEER_Q38_FULL_OFFLOAD_BATCHED != 1
#error Full offload requires exact batched selection,not defined zero
#endif
static constexpr uint64_t BuildTag=UINT64_C(0x2026110400010089);
#else
static constexpr uint64_t BuildTag=UINT64_C(0x2026110400010088);
#endif
enum {Version=1,MaxPrompt=96,MaxOutput=32,MaxPositions=128,Vocabulary=248320};
#ifdef PIONEER_Q38_FULL_OFFLOAD_METRICS
using CounterPacket=PioneerQ38TokenFrontierControl::Packet;
static inline bool CounterRequest(const CounterPacket& p){return PioneerQ38TokenFrontierControl::RequestShape(p);}
static inline bool CounterReply(const CounterPacket& p){
 return PioneerQ38TokenFrontierControl::ReplyShape(p) && p.reply.ready==1
  && p.reply.detail[8]<=MaxPositions && p.reply.detail[9]<=MaxPrompt
  && p.reply.detail[8]-p.reply.detail[9]+1<=MaxOutput;
} // integer DATA only;not terminal,HOLD,token readout,cleanup or rate authority
#endif
enum {Matrices=1,Attention=2,Recurrent=4,KV=8,Embedding=16,Normalize=32,
 Residual=64,Activation=128,Greedy=256,AllCompute=511};
struct Support {
 uint32_t driver,version,prompt,output,positions,vocabulary,compute,reserved;
};
static_assert(sizeof(Support)==32,"pointer-free full-offload capabilities");
static inline bool SupportRequest(const Support& s){
 return !s.driver && !s.version && !s.prompt && !s.output && !s.positions
  && !s.vocabulary && !s.compute && !s.reserved;
}
static inline bool Supported(const Support& s){
 return s.driver==55 && s.version==Version && s.prompt==MaxPrompt
  && s.output==MaxOutput && s.positions==MaxPositions && s.vocabulary==Vocabulary
  && s.compute==AllCompute && !s.reserved;
}
static inline bool RequestShape(const PioneerQ38TokenControl::Packet& p){
 namespace P=PioneerQ38TokenControl;
 if(!P::RequestShape(p))return false;
 const auto& r=p.request;
 // The final prompt evaluation predicts the first output. Hence output
 // count is limit-prompts+1, not limit-prompts. EOS is mandatory.
 if(r.operation==P::Start)return r.prompts<=MaxPrompt && r.limit<=MaxPositions
  && r.limit-r.prompts+1<=MaxOutput && r.stopEos==1
  && r.eos0==PioneerQ38FullOffloadEog::Eos0 && r.eos1==PioneerQ38FullOffloadEog::Eos1;
 return true;
}
}
#endif
