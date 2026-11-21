/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowBlackholeIO.h"
#include "../PioneerQwen35DataflowTensixOps.h"
#include "../PioneerQwen35DataflowBlackholeWorker.h"
#include "../PioneerQwen35DataflowBlackholeEmbeddingWorker.h"
#include "../PioneerQwen35DataflowVectorUnpack.h"
#include "../PioneerQwen35DataflowEmbedding.h"
#include "../PioneerQwen35DataflowEmbeddingProgram.h"
#include "../PioneerQwen35DataflowRms.h"
#include "../PioneerQwen35DataflowRmsProgram.h"
#include "../PioneerQwen35DataflowBlackholeRmsWorker.h"
#include "../PioneerQwen35DataflowSilu.h"
#include "../PioneerQwen35DataflowSiluProgram.h"
#include "../PioneerQwen35DataflowBlackholeSiluWorker.h"
#include "../PioneerQwen35DataflowResidualProgram.h"
#include "../PioneerQwen35DataflowBlackholeResidualWorker.h"
#include "../PioneerQwen35DataflowFfnSequence.h"
#include "../PioneerQwen35DataflowRecurrentLayerSequence.h"
#include "../PioneerQwen35DataflowDenseLayerSequence.h"
#include "../PioneerQwen35DataflowDecoderSequence.h"
#include "../PioneerQwen35DataflowBlackholeArgmaxWorker.h"
#include "../PioneerQwen35DataflowGenerationSequence.h"

using namespace PioneerQwen35;

static DataflowProjectionProgram sDataflowProjectionScratch;

struct TargetFfnAuthority {
	bool ResetRecurrentState() { return true; }
	bool BuildEmbedding(unsigned,unsigned,DataflowEmbeddingProgram&) { return true; }
	bool BuildArgmax(DataflowArgmaxProgram&) { return true; }
	bool BuildResidual(unsigned,unsigned,DataflowResidualProgram&) { return true; }
	bool BuildRms(unsigned,unsigned,unsigned,unsigned,DataflowRmsProgram&) { return true; }
	bool BuildProjection(unsigned,unsigned,unsigned,unsigned,
		DataflowProjectionProgram&) { return true; }
	bool BuildConv(unsigned,unsigned,DataflowConvProgram&) { return true; }
	bool BuildDelta(unsigned,unsigned,DataflowDeltaProgram&) { return true; }
	bool BuildRecurrentOutput(unsigned,unsigned,DataflowProjectionProgram&)
		{ return true; }
	bool BuildDenseProjection(unsigned,unsigned,unsigned,
		DataflowProjectionProgram&) { return true; }
	bool BuildDenseAttention(unsigned,unsigned,unsigned,
		DataflowDenseAttentionProgram&) { return true; }
	bool BuildDenseAttentionOutput(unsigned,unsigned,
		DataflowProjectionProgram&) { return true; }
	bool BuildSilu(unsigned,DataflowSiluProgram&) { return true; }
};

struct TargetFfnService {
	int EmbedToken(const DataflowEmbeddingProgram&,int&) { return 0; }
	int RunArgmax(const DataflowArgmaxProgram&,unsigned& token,int&)
		{ token=0; return 0; }
	int RunResidual(const DataflowResidualProgram&,int&) { return 0; }
	int RunRms(const DataflowRmsProgram&,int&) { return 0; }
	int RunProjection(const DataflowProjectionProgram&,int&) { return 0; }
	int RunConv(const DataflowConvProgram&,int&) { return 0; }
	int RunDelta(const DataflowDeltaProgram&,int&) { return 0; }
	int RunDenseAttention(const DataflowDenseAttentionProgram&,int&) { return 0; }
	int RunSilu(const DataflowSiluProgram&,int&) { return 0; }
};

bool PioneerQwen35DataflowFfnSequenceTargetCompileProbe(unsigned layer,
	unsigned inputSlot)
{
	TargetFfnAuthority authority;
	TargetFfnService service;
	int io=0;
	unsigned outputSlot=ActivationBuffers;
	return DataflowFfnSequence::Run(layer,inputSlot,authority,service,io,
		outputSlot,sDataflowProjectionScratch)==DataflowFfnSequenceComplete;
}

bool PioneerQwen35DataflowRecurrentLayerSequenceTargetCompileProbe(
	unsigned layer,unsigned position,unsigned inputSlot)
{
	TargetFfnAuthority authority;
	TargetFfnService service;
	int io=0;
	unsigned outputSlot=ActivationBuffers,stage=0;
	return DataflowRecurrentLayerSequence::Run(layer,position,inputSlot,
		authority,service,io,outputSlot,stage,sDataflowProjectionScratch)
		==DataflowRecurrentLayerComplete;
}

bool PioneerQwen35DataflowDenseLayerSequenceTargetCompileProbe(
	unsigned layer,unsigned position,unsigned inputSlot)
{
	TargetFfnAuthority authority;
	TargetFfnService service;
	int io=0;
	unsigned outputSlot=ActivationBuffers,stage=0;
	return DataflowDenseLayerSequence::Run(layer,position,inputSlot,
		authority,service,io,outputSlot,stage,sDataflowProjectionScratch)
		==DataflowDenseLayerComplete;
}

bool PioneerQwen35DataflowDecoderSequenceTargetCompileProbe(unsigned position,
	unsigned token,unsigned inputSlot)
{
	TargetFfnAuthority authority;
	TargetFfnService service;
	int io=0;
	unsigned outputToken=Vocabulary,stage=0;
	return DataflowDecoderSequence::Run(position,token,inputSlot,authority,
		service,io,outputToken,stage,sDataflowProjectionScratch)
		==DataflowDecoderSequenceComplete;
}

bool PioneerQwen35DataflowBlackholeArgmaxTargetCompileProbe(
	const DataflowArgmaxInvocation& invocation)
{
	DataflowBlackholeArgmaxOps io;
	DataflowWorkerMailbox mailbox;
	DataflowArgmaxZeroWords(mailbox);
	return io.Bind(invocation) && PioneerQwen35DataflowArgmaxWorker(
		invocation,1,reinterpret_cast<float*>(static_cast<uintptr_t>(
			DataflowActivationCacheAddress)),mailbox,io);
}

bool PioneerQwen35DataflowGenerationSequenceTargetCompileProbe(unsigned token)
{
	struct Clock { uint64_t NowMicros() { return 0; } } clock;
	TargetFfnAuthority authority;
	TargetFfnService service;
	int io=0;
	const unsigned prompt[1]={token};
	unsigned output[1]{},outputCount=0,stage=0;
	uint64_t promptTime=0,decodeTime=0;
	return DataflowGenerationSequence::Run(prompt,1,1,Vocabulary,0,
		authority,service,io,output,outputCount,stage,clock,promptTime,decodeTime,
		sDataflowProjectionScratch)
		==DataflowGenerationSequenceComplete && outputCount==1;
}

bool PioneerQwen35DataflowSiluTargetCompileProbe(float gate,float up,
	float& output)
{
	return DataflowSiluProduct(gate,up,output);
}

bool PioneerQwen35DataflowSiluProgramTargetCompileProbe(
	const DataflowSiluProgram& program,unsigned core,
	DataflowSiluInvocation& invocation)
{
	return DataflowSiluProgramBuilder::BindCore(program,core,invocation);
}

bool PioneerQwen35DataflowBlackholeSiluTargetCompileProbe(
	const DataflowSiluInvocation& invocation)
{
	DataflowBlackholeSiluOps io;
	return io.Bind(invocation) && PioneerQwen35DataflowSiluWorker(invocation,
		reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowInputBufferAddress)),
		reinterpret_cast<float*>(static_cast<uintptr_t>(
			DataflowInputBufferAddress+DataflowInputPageBytes)),
		reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowOutputBufferAddress)),io);
}

bool PioneerQwen35DataflowResidualProgramTargetCompileProbe(
	const DataflowResidualProgram& program,unsigned core,
	DataflowResidualInvocation& invocation)
{
	return DataflowResidualProgramBuilder::BindCore(program,core,invocation);
}

bool PioneerQwen35DataflowBlackholeResidualTargetCompileProbe(
	const DataflowResidualInvocation& invocation)
{
	DataflowBlackholeResidualOps io;
	return io.Bind(invocation) && PioneerQwen35DataflowResidualWorker(invocation,
		reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowInputBufferAddress)),
		reinterpret_cast<float*>(static_cast<uintptr_t>(
			DataflowInputBufferAddress+DataflowInputPageBytes)),
		reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowOutputBufferAddress)),io);
}

bool PioneerQwen35DataflowDenseAttentionProgramTargetCompileProbe(
	const DataflowDenseAttentionProgram& program,unsigned core,
	DataflowDenseAttentionInvocation& invocation)
{
	return DataflowDenseAttentionProgramBuilder::BindCore(program,core,invocation);
}

bool PioneerQwen35DataflowBlackholeDenseAttentionTargetCompileProbe(
	const DataflowDenseAttentionInvocation& invocation)
{
	DataflowBlackholeDenseAttentionOps io;
	float* const norm=reinterpret_cast<float*>(static_cast<uintptr_t>(
		DataflowDenseNormAddress));
	return PioneerQwen35DataflowDenseAttentionWorker(invocation,
		reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowDenseKeyTileAddress)),
		reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowDenseValueTileAddress)),
		reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowDenseQueryAddress)),
		reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowDenseGateAddress)),
		norm,reinterpret_cast<float*>(static_cast<uintptr_t>(
			DataflowDenseRopeAddress)),norm,io);
}

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

bool PioneerQwen35DataflowRmsTargetCompileProbe(const float* input,
	const float* weight,float* output)
{
	return DataflowNormalizeHidden(input,weight,output);
}

bool PioneerQwen35DataflowRmsProgramTargetCompileProbe(
	const DataflowRmsProgram& program,unsigned core,
	DataflowRmsInvocation& invocation)
{
	return DataflowRmsProgramBuilder::BindCore(program,core,invocation);
}

bool PioneerQwen35DataflowBlackholeRmsTargetCompileProbe(
	const DataflowRmsInvocation& invocation,unsigned sequence)
{
	DataflowBlackholeRmsOps io;
	return io.Bind(invocation,sequence)
		&& PioneerQwen35DataflowRmsWorker(invocation,sequence,
			reinterpret_cast<float*>(static_cast<uintptr_t>(
				DataflowActivationCacheAddress)),
			reinterpret_cast<float*>(static_cast<uintptr_t>(
				DataflowInputBufferAddress)),
			reinterpret_cast<float*>(static_cast<uintptr_t>(
				DataflowWeightBufferAddress)),
			reinterpret_cast<float*>(static_cast<uintptr_t>(
				DataflowOutputBufferAddress)),io);
}

bool PioneerQwen35DataflowEmbeddingProgramTargetCompileProbe(
	const DataflowEmbeddingProgram& program,unsigned core,
	DataflowEmbeddingInvocation& invocation)
{
	return DataflowEmbeddingProgramBuilder::BindCore(program,core,invocation);
}

bool PioneerQwen35DataflowBlackholeEmbeddingTargetCompileProbe(
	const DataflowEmbeddingInvocation& program)
{
	DataflowBlackholeEmbeddingOps io;
	return io.Bind(program) && PioneerQwen35DataflowEmbedWorker(program,
		reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(DataflowWeightBufferAddress)),
		reinterpret_cast<float*>(static_cast<uintptr_t>(DataflowOutputBufferAddress)),io);
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
