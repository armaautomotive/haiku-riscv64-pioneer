/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowKernels.h"

using namespace PioneerQwen35;

// Compile-only device adapter. Its methods deliberately contain no inference
// math; this probe verifies that all role templates lower for the Work image's
// RV32IMF target without relying on the RISC-V A extension or libatomic.
struct TargetCompileOps {
	enum { TensixMatrixEngine=1 };
	bool ReadActivation(const RowSpace&,unsigned,uint8_t*,unsigned) { return true; }
	bool ReadWeight(unsigned,uint32_t,uint8_t*,unsigned) { return true; }
	bool PackActivation(unsigned,unsigned,unsigned,const float*,uint8_t*,unsigned)
		{ return true; }
	bool TensixBeginOutput(unsigned) { return true; }
	bool TensixMatmul(unsigned,unsigned,const uint8_t*,const uint8_t*) { return true; }
	bool TensixFinishOutput(uint8_t*,unsigned) { return true; }
	bool WriteOutput(unsigned,uint32_t,const uint8_t*,unsigned) { return true; }
};

bool PioneerQwen35DataflowTargetCompileProbe(
	const DataflowCoreInvocation& program,
	DataflowQueueView (&queues)[DataflowCircularBufferCount],
	uint8_t* activationCache,TargetCompileOps& ops)
{
	if (!PioneerQwen35DataflowReader(program,queues[0],queues[1],
		activationCache,ops)) return false;
	if (!PioneerQwen35DataflowCompute(program,queues[0],queues[1],queues[2],ops))
		return false;
	return PioneerQwen35DataflowWriter(program,queues[2],ops);
}
