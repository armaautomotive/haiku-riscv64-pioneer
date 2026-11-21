/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowBlackholeIO.h"
#include "../PioneerQwen35DataflowTensixOps.h"
#include "../PioneerQwen35DataflowBlackholeWorker.h"
#include "../PioneerQwen35DataflowVectorUnpack.h"
#include "../PioneerQwen35DataflowEmbedding.h"

using namespace PioneerQwen35;

// Compile-only device adapter. Its methods deliberately contain no inference
// math; this probe verifies that all role templates lower for the Work image's
// RV32IMF target without relying on the RISC-V A extension or libatomic.
struct TargetCompileOps {
	enum { TensixMatrixEngine=1 };
	bool Bind(const DataflowCoreInvocation&) { return true; }
	bool ReadActivation(const RowSpace&,unsigned,uint8_t*,unsigned) { return true; }
	bool ReadWeight(unsigned,uint32_t,uint8_t*,unsigned) { return true; }
	bool PackActivation(unsigned,unsigned,unsigned,const float*,uint8_t*,unsigned)
		{ return true; }
	bool TensixBeginOutput(unsigned) { return true; }
	bool TensixMatmul(unsigned,unsigned,const uint8_t*,const uint8_t*) { return true; }
	bool TensixFinishOutput(uint8_t*,unsigned) { return true; }
	bool WriteOutput(unsigned,uint32_t,const uint8_t*,unsigned) { return true; }
};

struct TargetCompileTensixEmitter {
	void Config(unsigned,uint32_t) {}
	void Op(uint32_t) {}
	void Drain() {}
};

bool PioneerQwen35DataflowQ5UnpackTargetCompileProbe(unsigned panel,
	unsigned weightPage)
{
	TargetCompileTensixEmitter emitter;
	return PioneerQwen35DataflowVectorUnpack(emitter,13,panel,weightPage);
}

bool PioneerQwen35DataflowEmbeddingTargetCompileProbe(const uint8_t* tile,
	unsigned lane,unsigned panel,float* output)
{
	return DataflowDecodeQ6KEmbeddingPanel(tile,DataflowQ6KEmbeddingTileBytes,
		lane,panel,output,DataflowQ6KEmbeddingValuesPerPanel);
}

bool PioneerQwen35DataflowTensixTargetCompileProbe(
	const DataflowCoreInvocation& program,
	DataflowQueueView (&queues)[DataflowCircularBufferCount])
{
	DataflowBlackholeTensixOps ops;
	return PioneerQwen35DataflowCompute(program,queues[0],queues[1],
		queues[2],ops);
}

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

// This second compile-only entry instantiates the real Blackhole NoC read and
// write bindings, including the separate NoC 0 reader and NoC 1 writer paths.
bool PioneerQwen35DataflowBlackholeTransferTargetCompileProbe(
	const DataflowCoreInvocation& program,
	DataflowQueueView (&queues)[DataflowCircularBufferCount],
	uint8_t* activationCache)
{
	DataflowBlackholeReaderOps reader;
	DataflowBlackholeWriterOps writer;
	return reader.Bind(program) && writer.Bind(program)
		&& PioneerQwen35DataflowReader(program,queues[0],queues[1],
			activationCache,reader)
		&& PioneerQwen35DataflowWriter(program,queues[2],writer);
}

// Compile the concrete one-BRISC-worker-per-core path that owns NoC reads,
// Tensix projection commands, and NoC writes for one immutable shard.
bool PioneerQwen35DataflowBlackholeWorkerTargetCompileProbe(
	const DataflowCoreInvocation& program,
	DataflowQueueView (&queues)[DataflowCircularBufferCount],
	uint8_t* activationCache)
{
	DataflowBlackholeReaderOps reader;
	DataflowBlackholeTensixOps compute;
	DataflowBlackholeWriterOps writer;
	return PioneerQwen35DataflowBlackholeWorker(program,queues,
		activationCache,reader,compute,writer);
}

[[noreturn]] void PioneerQwen35DataflowBlackholeWorkerServiceTargetCompileProbe()
{
	PioneerQwen35DataflowBlackholeWorkerService();
}
