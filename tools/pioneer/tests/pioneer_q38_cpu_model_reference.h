/* SPDX-License-Identifier: MIT */
// Check before the guard: late/defined-zero mixing must fail too.
#if !defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE) || PIONEER_Q38_FULL_MODEL_CPU_REFERENCE != 1 \
 || !defined(PIONEER_Q38_CPU_VALIDATION) || PIONEER_Q38_CPU_VALIDATION != 1
#error Exact separate whole-model CPU observer role required
#endif
#if defined(_KERNEL_MODE) || defined(__FAST_MATH__) || defined(PIONEER_ATTENTION_FREESTANDING) \
 || defined(PIONEER_Q38_WORK_INFERENCE) || defined(TT_PIONEER_Q38_WORK_MODE) || defined(PIONEER_Q38_NATIVE_Q8_LAYOUT) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_GLOBAL_CHAIN_F32_HIDDEN) || defined(PIONEER_Q38_TOKEN_CANCEL_BOUNDARY) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT) || defined(PIONEER_Q38_RECURRENT_PREFIX_VALIDATION_NATIVE) \
 || defined(PIONEER_Q38_RECURRENT_BLOCK_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_EMBEDDING_NATIVE) || defined(PIONEER_Q38_LEARNED_RMS_NATIVE) || defined(PIONEER_Q38_DECODER_RUNTIME) \
 || defined(PIONEER_Q38_MIXED_WORKER_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) \
 || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_ROOT_IMAGE) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_CONTEXT_COMPILE_ONLY) \
 || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_FLOW_FIXTURE) || defined(PIONEER_Q38_TOKEN_NATIVE_FULL_END_FLOW_FIXTURE) \
 || defined(PIONEER_Q38_NATIVE_STATE_TEST_BRIDGE) || defined(PIONEER_Q38_TOKEN_NATIVE_STATE_SNAPSHOT_FIXTURE)
#error CPU whole-model products must NEVER enter kernel,image,native,old debug or Work code
#endif
#ifndef PIONEER_Q38_CPU_MODEL_REFERENCE_H
#define PIONEER_Q38_CPU_MODEL_REFERENCE_H
#include "pioneer_q38_original_rows.h"
#include <memory>
#include <vector>
struct Q38CpuTokenTrace {
 unsigned position=0,input=0,selected=0,runnerUp=0;
 uint32_t best=0,second=0;
 std::vector<uint32_t> hidden; // embedding then64 layer outputs; CPU-private
 std::vector<uint32_t> logits;
};
class Q38CpuModelReference {
 struct Impl;std::unique_ptr<Impl> impl;
public:
 Q38CpuModelReference();~Q38CpuModelReference();
 Q38CpuModelReference(const Q38CpuModelReference&)=delete;
 Q38CpuModelReference& operator=(const Q38CpuModelReference&)=delete;
 bool Bind(Q38OriginalRows&);bool Step(unsigned,Q38CpuTokenTrace&);
 // CPU-private COPY only,never a mutable view or device-product supplier.
 // kind0:GDN FP32 after2/3 steps;kind1:Conv FP32 after3;kind2/3:packed
 // binary16 K/V after3. Invalid requests leave output and state unchanged.
 bool Snapshot(unsigned completed,unsigned layer,unsigned kind,std::vector<uint32_t>&);
 // Separate EOS-two-position validation COPY. No EOS decision is supplied
 // to an accelerator; all four state kinds are observed only at position2.
 // The normal three-position corpus and Snapshot restrictions stay intact.
 bool SnapshotEos(unsigned layer,unsigned kind,std::vector<uint32_t>&);
 bool Ready()const;
};
#endif
