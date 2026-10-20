/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_DELTA_PROBE_PROTOCOL_H
#define PIONEER_Q38_DELTA_PROBE_PROTOCOL_H
// Debug LOCAL qualification only. No user code/physical address/resident
// offset/DMA/clock/reset selector, and no claim of model inference.
#define TT_PCI_PROBE_Q38_DELTA_SUPPORT (B_DEVICE_OP_CODES_END + 0x55d0)
#define TT_PCI_PROBE_Q38_DELTA_RUN (B_DEVICE_OP_CODES_END + 0x55d1)
#define TT_PCI_PROBE_Q38_DELTA_ACCEPT (B_DEVICE_OP_CODES_END + 0x55d2)
enum { TT_Q38_DELTA_PROBE_VERSION=2 }; // NEW safe-terminal image only
static inline bool PioneerQ38DeltaProbeTerminal(uint32_t pc)
{
 return pc==0x72038 || pc==0x7203c; // BOTH sealed instructions jump to PARK;fault60/7c NEVER accepted
}
struct tt_q38_delta_support { uint32_t version,driver,local_only,keys,values,state_words,accept_required; };
struct tt_q38_delta_result {
 uint32_t version,state,prepared,started,completed,stopped,recovery_required;
 uint32_t nonce,phase,count,marker,pc[2],before[8],after[8],polls;
};
struct tt_q38_delta_probe {
 uint32_t version,reserved[3],decay,beta;
 uint32_t prior[8192],query[128],key[128],value[64];
 tt_q38_delta_result result;
 uint32_t next[8192],output[64];
};
struct tt_q38_delta_accept { uint32_t version,nonce,reserved[2]; };
static inline bool PioneerQ38DeltaProbeNormal(uint32_t word)
{
 return (word&0x7f800000)!=0x7f800000 && (!(word&0x7fffffff) || (word&0x7f800000));
}
static inline bool PioneerQ38DeltaProbeGate(uint32_t word)
{
 return !(word&0x80000000) && word<=0x3f800000 && PioneerQ38DeltaProbeNormal(word);
}
static inline bool PioneerQ38DeltaProbeValid(const tt_q38_delta_probe& r)
{
 if (r.version!=TT_Q38_DELTA_PROBE_VERSION || r.reserved[0] || r.reserved[1] || r.reserved[2]
  || !PioneerQ38DeltaProbeGate(r.decay) || !PioneerQ38DeltaProbeGate(r.beta)) return false;
 for (auto v:r.prior) if (!PioneerQ38DeltaProbeNormal(v)) return false;
 for (auto v:r.query) if (!PioneerQ38DeltaProbeNormal(v)) return false;
 for (auto v:r.key) if (!PioneerQ38DeltaProbeNormal(v)) return false;
 for (auto v:r.value) if (!PioneerQ38DeltaProbeNormal(v)) return false;
 return true;
}
#endif
