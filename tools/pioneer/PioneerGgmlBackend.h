/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GGML_BACKEND_H
#define PIONEER_GGML_BACKEND_H
#include "ggml-backend.h"
#include <stdint.h>
// Standalone experimental ACCEL backend. Host storage mirrors immutable
// Q8_0 weights; supported learned matrix operations execute on the card.
// Norms/attention/activations remain CPU operations, not hidden fallbacks.
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
	uint64_t tensors,bytes,uploads,matvecs;
	double weightSeconds,inputSeconds,matvecSeconds,diagnosticSeconds;
	bool failed;
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
