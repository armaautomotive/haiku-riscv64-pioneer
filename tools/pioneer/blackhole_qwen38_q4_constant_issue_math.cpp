/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_Q4_CONSTANT_ISSUE_PARENT_BINDING 1
// VECTOR_INLINE is defined by the returning resident service before the
// actual VectorPanel include, not a caller-supplied alternate emitter.
#include "blackhole_qwen38_twin_batch_prefetch_math.cpp"
static_assert(PioneerQ38Q4ConstantIssueSelection == 1,
	"distinct constant Q4 issue TU cannot silently fall back");
