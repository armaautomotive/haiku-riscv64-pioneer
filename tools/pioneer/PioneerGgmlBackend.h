/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GGML_BACKEND_H
#define PIONEER_GGML_BACKEND_H
#include "ggml-backend.h"
#include <stdint.h>
// Standalone experimental ACCEL backend. Host storage mirrors immutable
// Q8_0 weights; supported learned matrix operations execute on the card.
// Norms/attention/activations remain CPU operations, not hidden fallbacks.
ggml_backend_reg_t PioneerGgmlReg();
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
// Public bootstrap graph diagnostics only. Reference capture and comparison
// are separate observers; neither changes tensor data or numerical gates.
void PioneerGgmlTrace(bool enabled);
void PioneerGgmlBeginPosition(unsigned position);
bool PioneerGgmlCompareTap(ggml_tensor* tensor,bool ask,void*);
#endif
