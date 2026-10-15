/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_GGML_BACKEND_H
#define PIONEER_GGML_BACKEND_H
#include "ggml-backend.h"
#include <stdint.h>
// Standalone experimental ACCEL backend. Host storage mirrors immutable
// Q8_0 weights; supported learned matrix operations execute on the card.
// Norms/attention/activations remain CPU operations, not hidden fallbacks.
ggml_backend_reg_t PioneerGgmlReg();
struct PioneerGgmlStats { uint64_t tensors,bytes,uploads,matvecs; bool failed; };
bool PioneerGgmlSnapshot(PioneerGgmlStats& stats);
bool PioneerGgmlFinish();
#endif
