/* SPDX-License-Identifier: MIT */
// Consistency-checked failure DATA only. A failed/partial user copy is NOT
// atomic or authenticated;these words never authorize IO,cleanup or retry.
#if !defined(PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT) || PIONEER_Q38_TOKEN_BOUNDARY_VALIDATION_CLIENT != 1 \
 || defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_TOKEN_FAULT_VALIDATION_NATIVE) || defined(PIONEER_Q38_TOKEN_FAULT_PROGRAMS) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE)
#error Exact USER-only boundary failure DATA reader required
#endif
#ifndef PIONEER_Q38_TOKEN_BOUNDARY_FAULT_DATA_H
#define PIONEER_Q38_TOKEN_BOUNDARY_FAULT_DATA_H
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38TokenHiddenValidationProtocol.h"
#include <cstring>
struct PioneerQ38TokenBoundaryFaultWords {uint32_t error=0,savedLink=0;};
static inline bool PioneerQ38TokenBoundaryCopyFaultData(unsigned op,const void* data,size_t bytes,
 PioneerQ38TokenBoundaryFaultWords& output){
 namespace P=PioneerQ38TokenControl;
 if(op!=TT_PCI_PROBE_Q38_TOKEN_HIDDEN_CONTROL || !data || bytes!=sizeof(P::Packet))return false;
 P::Request q;P::Reply r;const auto* raw=static_cast<const unsigned char*>(data);
 std::memcpy(&q,raw,sizeof(q));std::memcpy(&r,raw+offsetof(P::Packet,reply),sizeof(r));
 if(q.magic!=P::Magic || q.version!=1 || q.operation!=P::Observe || q.prompts || q.limit || q.stopEos
  || q.eos0 || q.eos1 || q.records)return false;
 for(auto word:q.reserved)if(word)return false;
 if(r.version!=1 || r.operation!=P::Observe || r.started!=1 || r.ready || r.unsafe!=1 || r.complete
  || r.stage!=9 || r.records || !r.status)return false;
 for(auto word:r.reserved)if(word)return false;
 const auto* d=r.detail;
 if(d[0]!=1 || d[1]!=1 || d[2]!=9 || d[3] || d[4] || d[5]!=1 || d[6] || d[7]
  || d[8] || d[9] || d[10] || d[11] || d[12] || d[13] || d[14]!=0x51465431)return false;
 output={d[14],d[15]};return true;
} // readable USER packet required;unaligned bytes are copied,never dereferenced as a struct
#endif
