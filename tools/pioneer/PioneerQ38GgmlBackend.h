/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_GGML_BACKEND_H
#define PIONEER_Q38_GGML_BACKEND_H
#include "ggml-backend.h"
#include "PioneerQwen38Gguf.h"
// Single-context, single-threaded setup. Native callers provide ONLY the
// fully same-FD-authenticated public source and its exclusive52 session.
// This bridge has NO FD/ioctl,CPU products,reference validation or fallback.
struct PioneerQ38GgmlDispatch {
	void* owner=nullptr;
	const PioneerQwen38Gguf* model=nullptr;
	bool (*ready)(void*)=nullptr;
	bool (*multiply)(void*,unsigned,const float*,unsigned,float*,unsigned)=nullptr;
	bool (*finish)(void*)=nullptr;
	void (*abort)(void*)=nullptr;
};
bool PioneerQ38GgmlConfigure(const PioneerQ38GgmlDispatch& dispatch);
ggml_backend_reg_t PioneerQ38GgmlReg();
bool PioneerQ38GgmlFinish();
struct PioneerQ38GgmlStats { uint64_t projections=0; unsigned recognizedWeights=0; bool failed=false,finished=false,clean=false; };
PioneerQ38GgmlStats PioneerQ38GgmlSnapshot();
#endif
