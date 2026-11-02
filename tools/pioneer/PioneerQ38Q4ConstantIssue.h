/* SPDX-License-Identifier: MIT */
#include "PioneerQ38Q4ConstantIssueRole.h"
#ifndef PIONEER_Q38_Q4_CONSTANT_ISSUE_H
#define PIONEER_Q38_Q4_CONSTANT_ISSUE_H
#ifndef PIONEER_Q38_Q4_CONSTANT_ISSUE_IMAGE
#error Separate full-worker constant Q4 BUILD only
#endif
// Called only AFTER VectorPanel validated the actual panel. Transform uses
// only its nibble bit: rows/parities, raw loads, original 2/3-op decoder,
// replay slots, native stores and MOVD2A are otherwise identical. A constant
// Panel makes the existing emitter issue constant Tensix instructions
// inline instead of runtime writes to the instruction-buffer MMIO port.
// No new ISA, formats, arithmetic, scratch, cache, drain or bank handoff.
// The caller retains the original final real Drain and all ownership checks.
template<class E, unsigned Panel>
static __attribute__((noinline)) bool PioneerQ38Q4ConstantTransform(E& e)
{
	static_assert(Panel == 0 || Panel == 2, "only the two existing Q4 decoders");
	const unsigned replay = 0x04000000 | ((2 + unsigned(Panel == 2)) << 4);
	if (!PioneerQ38VectorChunk<E,0,0>(e,12,Panel,false,replay)
	 || !PioneerQ38VectorChunk<E,0,2>(e,12,Panel,false,replay)
	 || !PioneerQ38VectorChunk<E,4,0>(e,12,Panel,false,replay)
	 || !PioneerQ38VectorChunk<E,4,2>(e,12,Panel,false,replay)
	 || !PioneerQ38VectorChunk<E,8,0>(e,12,Panel,false,replay)
	 || !PioneerQ38VectorChunk<E,8,2>(e,12,Panel,false,replay)
	 || !PioneerQ38VectorChunk<E,12,0>(e,12,Panel,false,replay)
	 || !PioneerQ38VectorChunk<E,12,2>(e,12,Panel,false,replay)) return false;
	e.Op(0x08002040); e.Op(0x08082044);
	e.Op(0x08102048); e.Op(0x0818204c);
	return true;
}
#endif
