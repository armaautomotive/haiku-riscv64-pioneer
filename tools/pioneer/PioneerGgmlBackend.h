/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GGML_BACKEND_H
#define PIONEER_GGML_BACKEND_H
#include "ggml-backend.h"
#include <stdint.h>
// Standalone experimental ACCEL backend. Host storage mirrors immutable
// Q8_0 weights; supported learned matrix operations execute on the card.
// Norms/activations remain host operations. Attention is host by default,
// or explicitly dispatched to the separately qualified bounded card mode.
ggml_backend_reg_t PioneerGgmlReg();
// A registered ACCEL device is initialized even for CPU-only llama contexts.
// Disable execution during their creation: they receive an inert backend
// that supports NO operations and can never open the card. Its default
// host-storage buffer remains accepted to satisfy ggml's scheduler contract.
void PioneerGgmlContextExecution(bool enabled);
typedef bool (*PioneerGgmlBoundaryCallback)(void*);
// Called only between completed uploads/matvecs, never during an ioctl.
void PioneerGgmlBoundary(PioneerGgmlBoundaryCallback callback,void* data);
struct PioneerGgmlStats {
	uint64_t tensors,bytes,uploads,matvecs,attentionOps,attentionTiles;
	double weightSeconds,inputSeconds,matvecSeconds,diagnosticSeconds,attentionSeconds;
	bool failed;
	unsigned peakWorkers;
};
bool PioneerGgmlSnapshot(PioneerGgmlStats& stats);
bool PioneerGgmlFinish();
// Expensive independent primitive diagnostics, not a production benchmark.
// Configure before creating a context; results never replace device output.
// Optional output path must stay alive; exclusive-create writes only the
// first failing primitive's FP32 input vector, never overwriting a file.
void PioneerGgmlDiagnose(bool enabled,const char* vectorPath=NULL);
// CPU-context-only eval tap; never attach this to the P100A context.
bool PioneerGgmlReferenceTap(ggml_tensor* tensor,bool ask,void*);
size_t PioneerGgmlReferenceCount();
void PioneerGgmlCpuActivationQuantization(bool enabled);
void PioneerGgmlFp16Operands(bool enabled);
void PioneerGgmlQ8Operands(bool enabled);
void PioneerGgmlQ8Compact(bool enabled);
void PioneerGgmlQ8Native(bool enabled);
void PioneerGgmlQ8Exact(bool enabled);
void PioneerGgmlQ8Dual(bool enabled);
void PioneerGgmlQ8DualReuse(bool enabled);
// Explicit bounded Qwen0.6B fused attention; requires exact dual-reuse mode.
// PIO-staged prefix, not persistent device KV or a speedup qualification.
void PioneerGgmlAttention(bool enabled);
// 0: legacy; 1: require bulk capability; 2: prefer it when advertised at
// initial open, before BEGIN. Never switch/fall back after any transfer error.
void PioneerGgmlBulkUpload(unsigned mode);
// Separate throughput candidate: whole projections, shared device input,
// up to120 workers and one call per complete grouped-head attention op.
// Still host norms/RoPE/KV; NOT a device-resident decoder or speed guarantee.
void PioneerGgmlFleet(unsigned workers);
// Explicit separate image/ABI, private28MiB tail; host prefix coherence
// bookkeeping, new-token-only transfers during ordinary sequential decode.
// This is not a full device-resident decoder or CPU-speedup guarantee.
void PioneerGgmlResidentKV(bool enabled);
// Separate opt-in: bounded SRAM-data transfers use phase fences rather
// than serializing every DWORD. All control/NoC/reset fences stay unchanged.
void PioneerGgmlPayloadBlocks(bool enabled);
// Explicit separate mode8: master dispatcher + <=119 persistent math
// peers, device KV-v3 handoff. No CPU reference/fallback or speed guarantee.
void PioneerGgmlCoordinated(bool enabled);
// Public bootstrap graph diagnostics only. Reference capture and comparison
// are separate observers; neither changes tensor data or numerical gates.
void PioneerGgmlTrace(bool enabled);
void PioneerGgmlBeginPosition(unsigned position);
bool PioneerGgmlCompareTap(ggml_tensor* tensor,bool ask,void*);
// CPU model/context and its immutable weights must remain alive. Replays
// only one captured public-test projection, never a generated conversation.
bool PioneerGgmlReplayReference(const char* tensorName,bool roundingModel=false);
// CPU-only diagnostic control; never opens or computes on P100A.
bool PioneerGgmlReplayCpuRope(const char* tensorName);
bool PioneerGgmlReplayRoundingReference(const char* tensorName);
#endif
