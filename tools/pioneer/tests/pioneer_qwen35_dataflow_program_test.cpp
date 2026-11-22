/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowKernels.h"
#include "../PioneerQwen35DataflowVectorUnpack.h"
#include "../PioneerQwen35DataflowAuthorization.h"
#include "../PioneerQwen35DataflowRoleCoordinator.h"
#include "../PioneerQwen35DataflowSharedActivationRoles.h"
#include "../PioneerQ38ProjectionInput.h"
#include <assert.h>
#include <atomic>
#include <cstring>
#include <cstdio>
#include <thread>

using namespace PioneerQwen35;

// Offline geometry only; the future owner must supply its independently
// audited route table for each NoC before these descriptors can run on-card.
static const unsigned TestBankRoutes[2][Banks]={
	{786,978,1170,785,977,1169,1361},
	{786,978,1170,785,977,1169,1361}
};

static bool BuildProgram(const TensorSpace& weight,const RowSpace& input,
	unsigned inputRows,const RowSpace& output,DataflowProjectionProgram& result)
{
	return DataflowProgramBuilder::Build(weight,input,inputRows,output,
		TestBankRoutes,result);
}

struct ContractOps {
	enum { TensixMatrixEngine=1 };
	bool Bind(const DataflowCoreInvocation&) { return true; }
	bool ReadActivation(const RowSpace&,unsigned,uint8_t*,unsigned) { return false; }
	bool ReadWeight(unsigned,uint32_t,uint8_t*,unsigned) { return false; }
	bool PackActivation(unsigned,unsigned,unsigned,const float*,uint8_t*,unsigned) { return false; }
	bool TensixBeginOutput(unsigned) { return false; }
	bool TensixMatmul(unsigned,unsigned,const uint8_t*,const uint8_t*) { return false; }
	bool TensixFinishOutput(uint8_t*,unsigned) { return false; }
	bool WriteOutput(unsigned,uint32_t,const uint8_t*,unsigned) { return false; }
};

struct PackTestOps {
	bool PackActivation(unsigned type,unsigned inputs,unsigned chunk,
		const float* source,uint8_t* output,unsigned bytes)
	{
		if ((type!=8 && type!=12 && type!=14)
			|| inputs%256 || chunk>=inputs/256 || bytes!=DataflowInputPageBytes
			|| source[chunk*256]!=float(chunk*256))
			return false;
		for (unsigned i=0;i<bytes;i++) output[i]=uint8_t(chunk);
		output[bytes-1]=uint8_t(chunk^0x5a);
		return true;
	}
};

static void CheckPackedActivationPages(const uint8_t* cache,unsigned type,unsigned inputs)
{
	const unsigned blocks=inputs/(type==8?32U:256U);
	assert(!DataflowPackedActivationPage(nullptr,type,inputs,0));
	assert(!DataflowPackedActivationPage(cache,type,inputs,blocks));
	for (unsigned block=0;block<blocks;block++) {
		const unsigned chunk=type==8?block/8:block;
		const uint8_t* page=DataflowPackedActivationPage(cache,type,inputs,block);
		assert(page==cache+chunk*DataflowInputPageBytes);
		for (unsigned i=0;i<DataflowInputPageBytes-1;i++)
			assert(page[i]==uint8_t(chunk));
		assert(page[DataflowInputPageBytes-1]==uint8_t(chunk^0x5a));
	}
}

struct UnpackAddressOps {
	unsigned addresses[2]{},count=0;
	void Config(unsigned index,uint32_t value)
	{
		if (index==76 && value!=0x7070) {
			assert(count<2);
			addresses[count++]=(value+1)*16;
		}
	}
	void Op(uint32_t) {}
	void Drain() {}
};

struct RoleTestCounters {
	std::atomic<unsigned> activationReads{0},weightReads{0},packedChunks{0},binds{0};
	std::atomic<unsigned> outputBegins{0},matmuls{0},outputFinishes{0},outputWrites{0};
	std::atomic<bool> okay{true};
	void Fail() { okay.store(false,std::memory_order_relaxed); }
};

static float RoleActivationSample(unsigned row,unsigned generation)
{
	// Exactly representable mixed-sign inputs, with a changing vector on every
	// resident command and zero values among nonzero quantization blocks.
	const int sample=int((row*37U+generation*101U)%2047U)-1023;
	return float(sample)*0.03125f;
}

struct RoleTestOps {
	enum { TensixMatrixEngine=1 };
	RoleTestCounters& counters;
	bool bound=false;
	bool realPack=false;
	unsigned projectionType=13;
	unsigned generation=0;
	bool Bind(const DataflowCoreInvocation&) {
		if (bound) return false;
		bound=true; counters.binds.fetch_add(1,std::memory_order_relaxed); return true;
	}
	bool ReadActivation(const RowSpace& source,unsigned rows,uint8_t* output,unsigned bytes)
	{
		if (!bound || !output || rows>source.rows || source.rowBytes!=sizeof(float)
			|| bytes!=rows*sizeof(float)) { counters.Fail(); return false; }
		float* values=reinterpret_cast<float*>(output);
		for (unsigned i=0;i<rows;i++) values[i]=RoleActivationSample(i,generation);
		counters.activationReads.fetch_add(1,std::memory_order_relaxed);
		return true;
	}
	bool ReadWeight(unsigned bank,uint32_t address,uint8_t* output,unsigned bytes)
	{
		if (bank>=Banks || address<BankBase || address>=BankLimit || !output
			|| !bytes || bytes>DataflowMaxWeightPageBytes) {
			counters.Fail(); return false;
		}
		output[0]=uint8_t(bank^(address>>8));
		for (unsigned i=1;i<bytes;i++) output[i]=uint8_t(i+bank);
		counters.weightReads.fetch_add(1,std::memory_order_relaxed);
		return true;
	}
	bool PackActivation(unsigned type,unsigned inputs,unsigned chunk,
		const float* source,uint8_t* output,unsigned bytes)
	{
		if (type!=DataflowActivationPackingType(projectionType) || inputs!=Hidden || !source || !output
			|| chunk>=inputs/DataflowActivationChunkInputs
			|| bytes!=DataflowInputPageBytes
			|| source[chunk*DataflowActivationChunkInputs]
				!=RoleActivationSample(chunk*DataflowActivationChunkInputs,generation)) {
			counters.Fail(); return false;
		}
		if (realPack) {
			if (!PioneerQ38ProjectionQuantize(type,
				source+chunk*DataflowActivationChunkInputs,DataflowActivationChunkInputs,
				reinterpret_cast<PioneerQ38ProjectionInput*>(output))) {
				counters.Fail(); return false;
			}
		} else std::memset(output,int(uint8_t(chunk^generation)),bytes);
		counters.packedChunks.fetch_add(1,std::memory_order_relaxed);
		return true;
	}
	bool TensixBeginOutput(unsigned type)
	{
		if (type!=projectionType) { counters.Fail(); return false; }
		counters.outputBegins.fetch_add(1,std::memory_order_relaxed);
		return true;
	}
	bool TensixMatmul(unsigned type,unsigned block,const uint8_t* weight,
		const uint8_t* input)
	{
		if (type!=projectionType || !weight || !input
			|| (!realPack && input[0]!=uint8_t(DataflowActivationChunk(type,block)^generation))) {
			counters.Fail(); return false;
		}
		counters.matmuls.fetch_add(1,std::memory_order_relaxed);
		return true;
	}
	bool TensixFinishOutput(uint8_t* output,unsigned bytes)
	{
		if (!output || bytes!=DataflowOutputPageBytes) {
			counters.Fail(); return false;
		}
		std::memset(output,int(uint8_t(0x5a^generation)),bytes);
		counters.outputFinishes.fetch_add(1,std::memory_order_relaxed);
		return true;
	}
	bool WriteOutput(unsigned bank,uint32_t address,const uint8_t* input,unsigned bytes)
	{
		if (!bound || bank>=Banks || address<BankBase || address>=BankLimit || !input
			|| bytes!=DataflowOutputPageBytes || input[0]!=uint8_t(0x5a^generation)) {
			counters.Fail(); return false;
		}
		counters.outputWrites.fetch_add(1,std::memory_order_relaxed);
		return true;
	}
};

struct SharedComputeOps : RoleTestOps {
	const uint8_t* cache;
	DataflowQueueControl* ready;
	const PioneerQ38ProjectionInput* expected=nullptr;
	SharedComputeOps(RoleTestCounters& counters,const uint8_t* data,DataflowQueueControl* control)
		:RoleTestOps{counters},cache(data),ready(control) {}
	bool TensixMatmul(unsigned type,unsigned block,const uint8_t* weight,const uint8_t* input)
	{
		if (!ready || DataflowLoadAcquire(ready->produced)!=1
			|| DataflowLoadAcquire(ready->consumed)!=0
			|| input!=DataflowPackedActivationPage(cache,type,Hidden,block)) return false;
		if (realPack && (!expected || std::memcmp(input,
			&expected[DataflowActivationChunk(type,block)],sizeof(*expected)))) return false;
		return RoleTestOps::TensixMatmul(type,block,weight,input);
	}
};

struct RejectBindOps : RoleTestOps {
	bool Bind(const DataflowCoreInvocation&) { return false; }
};

static void CompileRoleInterfaces()
{
	DataflowCoreInvocation program{};
	DataflowQueueView queues[3]{};
	uint8_t activation[DataflowActivationCacheBytes]{};
	DataflowDeviceContext context{};
	ContractOps ops{};
	if (false) {
		DataflowBindLocalContext(context,DataflowFailureQueue);
		PioneerQwen35DataflowReader(program,queues[0],queues[1],activation,ops);
		PioneerQwen35DataflowCompute(program,queues[0],queues[1],queues[2],ops);
		PioneerQwen35DataflowWriter(program,queues[2],ops);
		PioneerQwen35DataflowReaderEntry(ops);
		PioneerQwen35DataflowComputeEntry(ops);
		PioneerQwen35DataflowWriterEntry(ops);
	}
}

static void Rows(RowSpace& space,unsigned rows,unsigned rowBytes,uint32_t base);

static void CheckDynamicWeightAddresses()
{
	const unsigned rawBases[]={DataflowWeightBufferAddress,
		DataflowWeightBufferAddress+DataflowWeightBufferPageStride};
	const unsigned types[]={8,12,13,14};
	for (unsigned rawBase:rawBases) {
		for (unsigned type:types) {
			const unsigned panels=type==8?2U:16U;
			for (unsigned panel=0;panel<panels;panel++) {
				PioneerQ38VectorPanelSpan span{};
				assert(type==13
					?PioneerQwen35DataflowQ5PanelSpan(panel,span)
					:PioneerQ38VectorPanel(type,panel,span));
				UnpackAddressOps ops{};
				assert(PioneerQwen35DataflowVectorUnpack(ops,type,panel,rawBase));
				assert(ops.count==(span.second?2U:1U));
				assert(ops.addresses[0]==rawBase+span.low);
				if (span.second) assert(ops.addresses[1]==rawBase+span.high);
			}
		}
	}
	PioneerQ38VectorPanelSpan q5Span{};
	assert(PioneerQwen35DataflowQ5PanelSpan(15,q5Span));
	UnpackAddressOps q5Scratch{};
	assert(!PioneerQwen35DataflowVectorUnpack(q5Scratch,13,15,
		DataflowQ5PanelScratchAddress));
	assert(q5Scratch.count==0);
	UnpackAddressOps rejected{};
	assert(!PioneerQwen35DataflowVectorUnpack(rejected,14,0,0x92000));
	assert(rejected.count==0);
	assert(!PioneerQwen35DataflowVectorUnpack(rejected,13,16,
		DataflowWeightBufferAddress));
}

static void CheckConcurrentRoles()
{
	TensorSpace weight{};
	weight.type=13;
	weight.inputs=Hidden;
	Rows(weight.place,Ffn,Hidden/256
		*(PioneerQwen35Projection::TileBytes(13)/16),0x08000000);
	RowSpace input{},output{};
	Rows(input,Ffn,sizeof(float),0x04000000);
	Rows(output,Ffn,sizeof(float),0x06000000);
	DataflowProjectionProgram program{};
	assert(BuildProgram(weight,input,Hidden,output,program));
	DataflowCoreInvocation invocation{};
	assert(DataflowProgramBuilder::BindCore(program,0,invocation));
	DataflowQueueControl controls[DataflowCircularBufferCount]{};
	uint8_t local[DataflowLocalBufferEnd-DataflowWeightBufferAddress]{};
	DataflowQueueView queues[DataflowCircularBufferCount]{};
	assert(DataflowBindQueues(invocation,controls,local,queues));
	alignas(float) uint8_t activation[DataflowActivationCacheBytes]{};
	RoleTestCounters counters{};
	{
		DataflowQueueControl rejectControls[DataflowCircularBufferCount]{};
		DataflowQueueView rejectQueues[DataflowCircularBufferCount]{};
		assert(DataflowBindQueues(invocation,rejectControls,local,rejectQueues));
		RejectBindOps reject{{counters}};
		assert(!PioneerQwen35DataflowReader(invocation,rejectQueues[0],
			rejectQueues[1],activation,reject));
		assert(DataflowQueueFailure(rejectControls[0])==DataflowFailureReader);
	}
	{
		DataflowQueueControl rejectControls[DataflowCircularBufferCount]{};
		DataflowQueueView rejectQueues[DataflowCircularBufferCount]{};
		assert(DataflowBindQueues(invocation,rejectControls,local,rejectQueues));
		RejectBindOps reject{{counters}};
		assert(!PioneerQwen35DataflowWriter(invocation,rejectQueues[2],reject));
		assert(DataflowQueueFailure(rejectControls[2])==DataflowFailureWriter);
	}
	RoleTestOps readerOps{counters},computeOps{counters},writerOps{counters};
	bool readerOkay=false,computeOkay=false,writerOkay=false;
	std::thread reader([&] { readerOkay=PioneerQwen35DataflowReader(invocation,
		queues[0],queues[1],activation,readerOps); });
	std::thread compute([&] { computeOkay=PioneerQwen35DataflowCompute(invocation,
		queues[0],queues[1],queues[2],computeOps); });
	std::thread writer([&] { writerOkay=PioneerQwen35DataflowWriter(invocation,
		queues[2],writerOps); });
	reader.join(); compute.join(); writer.join();
	if (!readerOkay || !computeOkay || !writerOkay
		|| !counters.okay.load(std::memory_order_relaxed))
		fprintf(stderr,"role test failed reader=%d compute=%d writer=%d okay=%d "
			"reads=%u/%u packs=%u matmuls=%u writes=%u fails=%u/%u/%u\n",
			readerOkay,computeOkay,writerOkay,counters.okay.load(),
			counters.activationReads.load(),counters.weightReads.load(),
			counters.packedChunks.load(),counters.matmuls.load(),
			counters.outputWrites.load(),DataflowQueueFailure(controls[0]),
			DataflowQueueFailure(controls[1]),DataflowQueueFailure(controls[2]));
	assert(readerOkay && computeOkay && writerOkay);
	assert(counters.okay.load(std::memory_order_relaxed));
	const unsigned groups=invocation.assignment.groups;
	const unsigned tiles=groups*invocation.blocks;
	assert(counters.binds==3 && counters.activationReads==1
		&& counters.packedChunks==Hidden/DataflowActivationChunkInputs
		&& counters.weightReads==tiles && counters.matmuls==tiles
		&& counters.outputBegins==groups && counters.outputFinishes==groups
		&& counters.outputWrites==groups);
	for (unsigned i=0;i<DataflowCircularBufferCount;i++)
		assert(DataflowLoadAcquire(controls[i].produced)
			==DataflowLoadAcquire(controls[i].consumed));
}

static void CheckCoordinatedRoles()
{
	TensorSpace weight{}; weight.type=13; weight.inputs=Hidden;
	Rows(weight.place,Ffn,Hidden/256*(PioneerQwen35Projection::TileBytes(13)/16),0x08000000);
	RowSpace input{},output{};
	Rows(input,Ffn,sizeof(float),0x04000000);
	Rows(output,Ffn,sizeof(float),0x06000000);
	DataflowProjectionProgram program{};
	assert(BuildProgram(weight,input,Hidden,output,program));
	DataflowCoreInvocation invocation{};
	assert(DataflowProgramBuilder::BindCore(program,0,invocation));
	DataflowQueueControl controls[DataflowCircularBufferCount]{};
	uint8_t local[DataflowLocalBufferEnd-DataflowWeightBufferAddress]{};
	DataflowQueueView queues[DataflowCircularBufferCount]{};
	alignas(float) uint8_t activation[DataflowActivationCacheBytes]{};
	assert(DataflowBindQueues(invocation,controls,local,queues));
	RoleTestCounters counters{};
	RoleTestOps readerOps{counters},computeOps{counters},writerOps{counters};
	DataflowRoleMailbox box{};
	box.magic=DataflowRoleMailboxMagic; box.version=DataflowRoleMailboxVersion;
	struct ReaderBody {
		const DataflowCoreInvocation& program; DataflowQueueView* queues;
		uint8_t* activation; RoleTestOps& ops;
		bool Run() { return PioneerQwen35DataflowReader(program,queues[0],queues[1],activation,ops); }
	} readerBody{invocation,queues,activation,readerOps};
	struct WriterBody {
		const DataflowCoreInvocation& program; DataflowQueueView* queues; RoleTestOps& ops;
		bool Run() { return PioneerQwen35DataflowWriter(program,queues[2],ops); }
	} writerBody{invocation,queues,writerOps};
	struct Coordinator {
		const DataflowCoreInvocation& program; DataflowQueueView* queues;
		DataflowQueueControl* controls; RoleTestOps& ops;
		bool RunCompute() { return PioneerQwen35DataflowCompute(program,queues[0],queues[1],queues[2],ops); }
		void Pause() { std::this_thread::yield(); }
		bool AuditAndResetCleanQueues() { return DataflowRoleCleanQueuesForReuse(controls); }
	} coordinator{invocation,queues,controls,computeOps};
	std::atomic<bool> bad{false};
	auto roleLoop=[&](unsigned role,auto& body) {
		DataflowRoleTickState state{};
		for (unsigned n=0;n<10000000 && !bad.load();n++) {
			const auto result=DataflowRoleServiceTick(box,role,state,body);
			if (result==DataflowRoleTickStopRequested) return;
			if (result==DataflowRoleTickInvalid || result==DataflowRoleTickFault
				|| result==DataflowRoleTickWaitingRecovery) { bad.store(true); return; }
			if ((n&255)==0) std::this_thread::yield();
		}
		bad.store(true);
	};
	std::thread reader([&] { roleLoop(DataflowRoleReader,readerBody); });
	std::thread writer([&] { roleLoop(DataflowRoleWriter,writerBody); });
	for (unsigned n=0;n<10000000 && !DataflowRoleMailboxAllStarted(box) && !bad.load();n++)
		if ((n&255)==0) std::this_thread::yield();
	assert(DataflowRoleMailboxAllStarted(box));
	const auto result=DataflowRoleCoordinate(box,1,DataflowQueuePollLimit,coordinator);
	if (result!=DataflowRoleCoordinatorComplete || !DataflowRoleMailboxRequestStop(box))
		bad.store(true);
	reader.join(); writer.join();
	assert(!bad.load() && DataflowRoleMailboxStopReceipts(box));
	assert(counters.okay && counters.binds==3 && counters.activationReads==1);
	assert(counters.matmuls==invocation.assignment.groups*invocation.blocks);
	assert(counters.outputWrites==invocation.assignment.groups);
	for (const auto& queue:controls) assert(!queue.produced && !queue.consumed);
}

static void CheckCoordinatedSharedActivationRoles(unsigned type,bool failReader=false)
{
	TensorSpace weight{}; weight.type=type; weight.inputs=Hidden;
	const unsigned blockInputs=type==8?32:256;
	Rows(weight.place,Ffn,Hidden/blockInputs*(PioneerQwen35Projection::TileBytes(type)/16),0x08000000);
	RowSpace input{},output{};
	Rows(input,Ffn,sizeof(float),0x04000000);
	Rows(output,Ffn,sizeof(float),0x06000000);
	DataflowProjectionProgram program{};
	assert(BuildProgram(weight,input,Hidden,output,program));
	DataflowCoreInvocation invocation{};
	assert(DataflowProgramBuilder::BindCore(program,0,invocation));
	const unsigned types[]={8,12,13,14};
	DataflowCoreInvocation variants[4]{};
	unsigned firstVariant=4;
	for (unsigned i=0;i<4;i++) {
		TensorSpace variant=weight;
		variant.type=types[i];
		const unsigned block=types[i]==8?32:256;
		Rows(variant.place,Ffn,Hidden/block*(PioneerQwen35Projection::TileBytes(types[i])/16),0x08000000);
		assert(BuildProgram(variant,input,Hidden,output,program));
		assert(DataflowProgramBuilder::BindCore(program,0,variants[i]));
		if (types[i]==type) firstVariant=i;
	}
	assert(firstVariant<4);
	DataflowQueueControl controls[DataflowCircularBufferCount]{};
	alignas(16) uint8_t local[DataflowLocalBufferEnd-DataflowWeightBufferAddress]{};
	DataflowQueueView queues[DataflowCircularBufferCount]{};
	alignas(float) uint8_t activation[DataflowActivationCacheBytes]{};
	assert(DataflowBindQueues(invocation,controls,local,queues));
	RoleTestCounters counters{};
	RoleTestOps readerOps{counters},writerOps{counters};
	SharedComputeOps computeOps{counters,activation,&controls[1]};
	PioneerQ38ProjectionInput expected[Hidden/DataflowActivationChunkInputs]{};
	readerOps.realPack=computeOps.realPack=true;
	computeOps.expected=expected;
	readerOps.projectionType=computeOps.projectionType=writerOps.projectionType=type;
	DataflowRoleMailbox box{};
	box.magic=DataflowRoleMailboxMagic; box.version=DataflowRoleMailboxVersion;
	std::atomic<unsigned> generation{0};
	struct ReaderProxy {
		RoleTestOps& ops; bool failWeight;
		bool Bind(const DataflowCoreInvocation& p) { return ops.Bind(p); }
		bool ReadActivation(const RowSpace& r,unsigned n,uint8_t* out,unsigned bytes)
			{ return ops.ReadActivation(r,n,out,bytes); }
		bool ReadWeight(unsigned bank,uint32_t address,uint8_t* out,unsigned bytes)
			{ return !(failWeight && ops.generation==8) && ops.ReadWeight(bank,address,out,bytes); }
		bool PackActivation(unsigned t,unsigned n,unsigned c,const float* in,uint8_t* out,unsigned bytes)
			{ return ops.PackActivation(t,n,c,in,out,bytes); }
	} readerProxy{readerOps,failReader};
	struct ReaderBody {
		const DataflowCoreInvocation& program; DataflowQueueView* queues;
		uint8_t* activation; ReaderProxy& ops; std::atomic<unsigned>& generation;
		bool Run() {
			ops.ops.bound=false; ops.ops.generation=generation.load(); ops.ops.projectionType=program.type;
			return PioneerQwen35SharedActivationReader(program,queues[0],queues[1],activation,ops);
		}
	} readerBody{invocation,queues,activation,readerProxy,generation};
	struct WriterBody {
		const DataflowCoreInvocation& program; DataflowQueueView* queues; RoleTestOps& ops;
		std::atomic<unsigned>& generation;
		bool Run() {
			ops.bound=false; ops.generation=generation.load(); ops.projectionType=program.type;
			return PioneerQwen35DataflowWriter(program,queues[2],ops);
		}
	} writerBody{invocation,queues,writerOps,generation};
	struct Coordinator {
		const DataflowCoreInvocation& program; DataflowQueueView* queues;
		DataflowQueueControl* controls; const uint8_t* activation; SharedComputeOps& ops;
		std::atomic<unsigned>& generation;
		bool RunCompute() {
			ops.bound=false; ops.generation=generation.load(); ops.projectionType=program.type;
			return PioneerQwen35SharedActivationCompute(program,queues[0],queues[1],queues[2],activation,ops);
		}
		void Pause() { std::this_thread::yield(); }
		bool AuditAndResetCleanQueues() { return DataflowRoleCleanQueuesForReuse(controls); }
	} coordinator{invocation,queues,controls,activation,computeOps,generation};
	std::atomic<bool> bad{false};
	auto roleLoop=[&](unsigned role,auto& body) {
		DataflowRoleTickState state{};
		for (unsigned n=0;n<10000000 && !bad.load();n++) {
			const auto result=DataflowRoleServiceTick(box,role,state,body);
			if (result==DataflowRoleTickStopRequested) return;
			if (result==DataflowRoleTickInvalid || (!failReader && (result==DataflowRoleTickFault
				|| result==DataflowRoleTickWaitingRecovery))) { bad.store(true); return; }
			if ((n&255)==0) std::this_thread::yield();
		}
		bad.store(true);
	};
	std::thread reader([&] { roleLoop(DataflowRoleReader,readerBody); });
	std::thread writer([&] { roleLoop(DataflowRoleWriter,writerBody); });
	for (unsigned n=0;n<10000000 && !DataflowRoleMailboxAllStarted(box) && !bad.load();n++)
		if ((n&255)==0) std::this_thread::yield();
	assert(DataflowRoleMailboxAllStarted(box));
	const unsigned runs=failReader?8:64;
	unsigned completedRuns=0,expectedMatmuls=0,expectedGroups=0;
	DataflowRoleCoordinatorResult result=DataflowRoleCoordinatorInvalid;
	for (unsigned run=0;run<runs;run++) {
		invocation=variants[(firstVariant+run)%4];
		assert(DataflowBindQueues(invocation,controls,local,queues));
		for (unsigned chunk=0;chunk<Hidden/DataflowActivationChunkInputs;chunk++) {
			float source[DataflowActivationChunkInputs];
			for (unsigned i=0;i<DataflowActivationChunkInputs;i++)
				source[i]=RoleActivationSample(chunk*DataflowActivationChunkInputs+i,run+1);
			assert(PioneerQ38ProjectionQuantize(DataflowActivationPackingType(invocation.type),
				source,DataflowActivationChunkInputs,&expected[chunk]));
		}
		generation.store(run+1);
		uint32_t next=0;
		if (!DataflowRoleNextProjectionSequence(box,next) || next!=run+1) { bad.store(true); break; }
		result=DataflowRoleCoordinate(box,next,DataflowQueuePollLimit,coordinator);
		if (result!=DataflowRoleCoordinatorComplete) break;
		completedRuns++;
		expectedMatmuls+=invocation.assignment.groups*invocation.blocks;
		expectedGroups+=invocation.assignment.groups;
	}
	if (failReader) {
		if (result!=DataflowRoleCoordinatorRetainedFault || !DataflowQueueFailure(controls[0])
			|| !DataflowRoleDiscardFailedQueues(controls,box,true)
			|| !DataflowRoleMailboxRequestFaultStop(box,true,true,true)) bad.store(true);
	} else if (result!=DataflowRoleCoordinatorComplete || !DataflowRoleMailboxRequestStop(box))
		bad.store(true);
	reader.join(); writer.join();
	assert(!bad.load() && DataflowRoleMailboxStopReceipts(box));
	assert(counters.okay && counters.binds==3*runs && counters.activationReads==runs);
	if (failReader) {
		assert(completedRuns==runs-1 && box.acknowledged==runs-1
			&& !DataflowQueueFailure(controls[0]));
	} else {
		assert(completedRuns==runs);
		for (const auto& queue:controls) assert(!queue.produced && !queue.consumed);
	}
	assert(counters.matmuls==expectedMatmuls && counters.outputWrites==expectedGroups);
}

static void CheckInactiveRoleReader()
{
	TensorSpace weight{};
	weight.type=13; weight.inputs=Hidden;
	Rows(weight.place,64,Hidden/256*(PioneerQwen35Projection::TileBytes(13)/16),0x08000000);
	RowSpace input{},output{};
	Rows(input,Hidden,sizeof(float),0x04000000);
	Rows(output,64,sizeof(float),0x06000000);
	DataflowProjectionProgram program{};
	assert(BuildProgram(weight,input,Hidden,output,program));
	DataflowCoreInvocation inactive{};
	bool found=false;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		assert(DataflowProgramBuilder::BindCore(program,core,inactive));
		if (!inactive.assignment.active) { found=true; break; }
	}
	assert(found && inactive.Valid());
	DataflowQueueControl controls[DataflowCircularBufferCount]{};
	uint8_t local[DataflowLocalBufferEnd-DataflowWeightBufferAddress]{};
	DataflowQueueView queues[DataflowCircularBufferCount]{};
	alignas(float) uint8_t activation[DataflowActivationCacheBytes]{};
	assert(DataflowBindQueues(inactive,controls,local,queues));
	RoleTestCounters counters{}; RoleTestOps reader{counters};
	assert(PioneerQwen35DataflowReader(inactive,queues[0],queues[1],activation,reader));
	assert(counters.binds==1 && counters.activationReads==0 && counters.weightReads==0
		&& counters.packedChunks==0);
}

static void Rows(RowSpace& space,unsigned rows,unsigned rowBytes,uint32_t base)
{
	space={};
	space.rows=rows;
	space.rowBytes=rowBytes;
	space.unit=16;
	const unsigned groups=rows/16;
	unsigned first=0;
	for (unsigned bank=0;bank<Banks;bank++) {
		const unsigned count=groups/Banks+unsigned(bank<groups%Banks);
		if (!count) continue;
		space.parts[space.count++]={bank,base+bank*0x01000000,first*16,count*16};
		first+=count;
	}
}

static void CheckTiedOutputProjection()
{
	DeviceLayout manifest{};
	manifest.magic=LayoutMagic;
	manifest.version=LayoutVersion;
	manifest.context=128;
	manifest.tied.type=14;
	manifest.tied.inputs=Hidden;
	Rows(manifest.tied.place,Vocabulary,2120,0x08000000);
	manifest.finalNorm.type=0;
	manifest.finalNorm.inputs=Hidden;
	manifest.finalNorm.place.rows=1;
	manifest.finalNorm.place.rowBytes=Hidden*sizeof(float);
	manifest.finalNorm.place.count=1;
	manifest.finalNorm.place.unit=1;
	manifest.finalNorm.place.parts[0]={2,BankBase+0x0a000000,0,1};
	assert(manifest.tied.place.Valid() && manifest.finalNorm.place.Valid());
	Rows(manifest.activation[0],Ffn,sizeof(float),0x04000000);
	Rows(manifest.activation[1],Ffn,sizeof(float),0x06000000);
	DataflowGraphWorkspace workspace{};
	Rows(workspace.space[DataflowGraphResidualWorkspaceIndex],Hidden,sizeof(float),
		0x20000000);
	Rows(workspace.space[DataflowGraphUpWorkspaceIndex],Ffn,sizeof(float),0x30000000);
	Rows(workspace.space[DataflowGraphRecurrentQkvWorkspaceIndex],ConvChannels,
		sizeof(float),0x40000000);
	Rows(workspace.space[DataflowGraphRecurrentGateWorkspaceIndex],DeltaInner,
		sizeof(float),0x50000000);
	Rows(workspace.space[DataflowGraphRecurrentAlphaWorkspaceIndex],DeltaHeads,
		sizeof(float),0x60000000);
	Rows(workspace.space[DataflowGraphRecurrentBetaWorkspaceIndex],DeltaHeads,
		sizeof(float),0x70000000);
	Rows(workspace.space[DataflowGraphVocabularyWorkspaceIndex],Vocabulary,
		sizeof(float),0x80000000);
	assert(workspace.Valid(manifest));

	DataflowProjectionAuthority authority;
	assert(authority.BindValidatedModel(manifest,workspace,TestBankRoutes));
	assert(authority.MarkActivationInitialized(0,Hidden));
	assert(authority.MarkActivationInitialized(1,Hidden));
	DataflowEmbeddingProgram embedding{};
	assert(authority.BuildEmbedding(42,0,embedding));
	assert(authority.BeginEmbedding(embedding)
		&& authority.PendingEmbeddingMatches(embedding)
		&& !authority.ActivationReady(0));
	assert(authority.AbortPendingProjection() && !authority.Pending()
		&& !authority.ActivationReady(0) && !authority.VocabularyReady());
	assert(authority.MarkActivationInitialized(0,Hidden));
	DataflowProjectionProgram tied{};
	assert(DataflowProgramBuilder::Build(manifest.tied,manifest.activation[1],Hidden,
		workspace.space[DataflowGraphVocabularyWorkspaceIndex],TestBankRoutes,tied));
	assert(tied.activeCores==DataflowCoreCount);
	assert(DataflowProjectionMatchesTiedOutput(manifest,workspace,TestBankRoutes,
		tied));
	assert(!authority.Authorize(tied));
	assert(!authority.BuildProjection(Layers,TiedOutput,1,
		DataflowGraphVocabularyOutputSlot,tied));

	DataflowRmsProgram finalNorm{};
	assert(authority.BuildRms(Layers,FinalNorm,0,1,finalNorm));
	assert(authority.BeginRms(finalNorm) && authority.CompleteRms(finalNorm));
	assert(authority.Authorize(tied));
	assert(authority.BuildProjection(Layers,TiedOutput,1,
		DataflowGraphVocabularyOutputSlot,tied));
	assert(tied.activeCores==DataflowCoreCount);
	unsigned active=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowCoreInvocation invocation{};
		assert(DataflowProgramBuilder::BindCore(tied,core,invocation));
		active+=invocation.assignment.active;
	}
	assert(active==DataflowCoreCount);
	assert(authority.Begin(tied) && authority.PendingProjectionMatches(tied));
	assert(authority.Complete(tied) && authority.VocabularyReady()
		&& authority.GraphWorkspaceLayer(DataflowGraphVocabularyWorkspaceIndex)==Layers
		&& authority.GraphWorkspaceRole(DataflowGraphVocabularyWorkspaceIndex)==TiedOutput);
	assert(!authority.BuildProjection(Layers,TiedOutput,1,
		DataflowGraphVocabularyOutputSlot,tied));
	DataflowArgmaxProgram argmax{};
	assert(authority.BuildArgmax(argmax) && argmax.Valid());
	assert(DataflowArgmaxMatchesWorkspace(manifest,workspace,TestBankRoutes,
		argmax));
	unsigned activeArgmax=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowArgmaxInvocation invocation{};
		assert(DataflowArgmaxProgramBuilder::BindCore(argmax,core,invocation));
		activeArgmax+=invocation.active;
	}
	assert(activeArgmax==1);
	assert(authority.BeginArgmax(argmax)
		&& authority.PendingArgmaxMatches(argmax));
	assert(authority.CompleteArgmax(argmax,12345)
		&& !authority.Pending() && !authority.VocabularyReady());
}

int main()
{
	CheckTiedOutputProjection();
#ifdef PIONEER_QWEN35_TIED_OUTPUT_ONLY
	return 0;
#endif
	CompileRoleInterfaces();
	CheckDynamicWeightAddresses();
	CheckConcurrentRoles();
	CheckCoordinatedRoles();
	for (unsigned type:{8U,12U,13U,14U}) CheckCoordinatedSharedActivationRoles(type);
	CheckCoordinatedSharedActivationRoles(13,true);
	CheckInactiveRoleReader();
	TensorSpace weight{};
	weight.type=14;
	weight.inputs=Hidden;
	Rows(weight.place,Ffn,2120,0x08000000);
	RowSpace input{},output{};
	Rows(input,Ffn,sizeof(float),0x04000000);
	Rows(output,Ffn,sizeof(float),0x06000000);

	DataflowProjectionProgram program{};
	assert(BuildProgram(weight,input,Hidden,output,program));
	assert(program.Valid());
	DeviceLayout manifest{};
	manifest.magic=LayoutMagic;
	manifest.version=LayoutVersion;
	manifest.context=128;
	manifest.tied=weight;
	manifest.activation[0]=input;
	manifest.activation[1]=output;
	TensorSpace& normWeight=manifest.layers[0].weights[AttentionNorm];
	normWeight.type=0;
	normWeight.inputs=Hidden;
	normWeight.place.rows=1;
	normWeight.place.rowBytes=Hidden*sizeof(float);
	normWeight.place.count=1;
	normWeight.place.unit=1;
	normWeight.place.parts[0]={2,BankBase+0x0a000000,0,1};
	assert(normWeight.place.Valid());
	TensorSpace& upWeight=manifest.layers[0].weights[Up];
	upWeight.type=12;
	upWeight.inputs=Hidden;
	Rows(upWeight.place,Ffn,
		PioneerQwen35Projection::TileBytes(upWeight.type)*(Hidden/256)/16,
		0x10000000);
	assert(upWeight.place.Valid());
	TensorSpace& recurrentQkv=manifest.layers[0].weights[RecurrentQkv];
	recurrentQkv.type=12;
	recurrentQkv.inputs=Hidden;
	Rows(recurrentQkv.place,ConvChannels,
		PioneerQwen35Projection::TileBytes(recurrentQkv.type)*(Hidden/256)/16,
		0x11000000);
	TensorSpace& recurrentGate=manifest.layers[0].weights[RecurrentGate];
	recurrentGate.type=12;
	recurrentGate.inputs=Hidden;
	Rows(recurrentGate.place,DeltaInner,
		PioneerQwen35Projection::TileBytes(recurrentGate.type)*(Hidden/256)/16,
		0x12000000);
	TensorSpace& recurrentAlpha=manifest.layers[0].weights[Alpha];
	recurrentAlpha.type=12;
	recurrentAlpha.inputs=Hidden;
	Rows(recurrentAlpha.place,DeltaHeads,
		PioneerQwen35Projection::TileBytes(recurrentAlpha.type)*(Hidden/256)/16,
		0x13000000);
	TensorSpace& recurrentBeta=manifest.layers[0].weights[Beta];
	recurrentBeta.type=12;
	recurrentBeta.inputs=Hidden;
	Rows(recurrentBeta.place,DeltaHeads,
		PioneerQwen35Projection::TileBytes(recurrentBeta.type)*(Hidden/256)/16,
		0x14000000);
	TensorSpace& denseQueryAndGate=manifest.layers[3].weights[QueryAndGate];
	denseQueryAndGate.type=12;
	denseQueryAndGate.inputs=Hidden;
	Rows(denseQueryAndGate.place,8192,
		PioneerQwen35Projection::TileBytes(denseQueryAndGate.type)*(Hidden/256)/16,
		0x15000000);
	TensorSpace& denseKey=manifest.layers[3].weights[Key];
	denseKey.type=12;
	denseKey.inputs=Hidden;
	Rows(denseKey.place,1024,
		PioneerQwen35Projection::TileBytes(denseKey.type)*(Hidden/256)/16,
		0x16000000);
	TensorSpace& denseValue=manifest.layers[3].weights[Value];
	denseValue.type=12;
	denseValue.inputs=Hidden;
	Rows(denseValue.place,1024,
		PioneerQwen35Projection::TileBytes(denseValue.type)*(Hidden/256)/16,
		0x17000000);
	assert(denseQueryAndGate.place.Valid() && denseKey.place.Valid()
		&& denseValue.place.Valid());
	assert(DataflowProjectionMatchesLayout(manifest,TestBankRoutes,program));
	DataflowGraphWorkspace graphWorkspace{};
	Rows(graphWorkspace.space[0],Hidden,sizeof(float),0x20000000);
	Rows(graphWorkspace.space[1],Ffn,sizeof(float),0x30000000);
	Rows(graphWorkspace.space[DataflowGraphRecurrentQkvWorkspaceIndex],ConvChannels,
		sizeof(float),0x40000000);
	Rows(graphWorkspace.space[DataflowGraphRecurrentGateWorkspaceIndex],DeltaInner,
		sizeof(float),0x50000000);
	Rows(graphWorkspace.space[DataflowGraphRecurrentAlphaWorkspaceIndex],DeltaHeads,
		sizeof(float),0x60000000);
	Rows(graphWorkspace.space[DataflowGraphRecurrentBetaWorkspaceIndex],DeltaHeads,
		sizeof(float),0x70000000);
	assert(graphWorkspace.Valid(manifest));
	DataflowProjectionAuthority authority;
	assert(authority.BindValidatedModel(manifest,graphWorkspace,TestBankRoutes));
	assert(authority.GraphWorkspace()
		&& DataflowRowSpacesEqual(authority.GraphWorkspace()->space[0],graphWorkspace.space[0]));
	assert(!authority.Authorize(program));
	assert(authority.MarkActivationInitialized(0));
	assert(authority.MarkActivationInitialized(1));
	assert(authority.Authorize(program));
	assert(authority.Begin(program));
	assert(authority.Pending() && authority.ActivationReady(0)
		&& !authority.ActivationReady(1));
	assert(!authority.Authorize(program));
	assert(authority.Complete(program));
	assert(!authority.Pending() && authority.ActivationReady(0)
		&& authority.ActivationReady(1)
		&& authority.ActivationRows(1)==program.outputRows);
	DataflowRmsProgram rmsProgram{};
	assert(authority.BuildRms(0,AttentionNorm,1,0,rmsProgram));
	assert(rmsProgram.Valid() && authority.AuthorizeRms(rmsProgram));
	assert(authority.BeginRms(rmsProgram));
	assert(authority.Pending() && !authority.ActivationReady(0)
		&& authority.PendingRmsMatches(rmsProgram)
		&& !authority.PendingEmbeddingMatches(DataflowEmbeddingProgram{}));
	assert(authority.CompleteRms(rmsProgram));
	assert(!authority.Pending() && authority.ActivationReady(0)
		&& authority.ActivationRows(0)==Hidden);
	DataflowProjectionProgram upProgram{};
	assert(authority.BuildProjection(0,Up,0,DataflowGraphUpOutputSlot,upProgram));
	assert(upProgram.Valid()
		&& DataflowProjectionMatchesGraphWorkspace(manifest,graphWorkspace,
			TestBankRoutes,upProgram));
	assert(!DataflowProjectionMatchesLayout(manifest,TestBankRoutes,upProgram));
	assert(authority.Begin(upProgram));
	assert(authority.Pending() && !authority.GraphUpReady()
		&& authority.PendingProjectionMatches(upProgram));
	assert(authority.Complete(upProgram));
	assert(!authority.Pending() && authority.GraphUpReady());
	DataflowProjectionProgram recurrentQkvProgram{};
	assert(authority.BuildProjection(0,RecurrentQkv,0,
		DataflowGraphRecurrentQkvOutputSlot,recurrentQkvProgram));
	unsigned graphInput=ActivationBuffers,graphIndex=DataflowGraphWorkspaceBufferCount;
	assert(DataflowProjectionMatchesGraphWorkspace(manifest,graphWorkspace,
		TestBankRoutes,recurrentQkvProgram,&graphInput,&graphIndex));
	assert(graphInput==0 && graphIndex==DataflowGraphRecurrentQkvWorkspaceIndex
		&& recurrentQkvProgram.outputRows==ConvChannels);
	assert(!authority.BuildProjection(0,RecurrentGate,0,
		DataflowGraphRecurrentQkvOutputSlot,recurrentQkvProgram));
	assert(authority.Begin(recurrentQkvProgram));
	assert(authority.Pending()
		&& !authority.GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex)
		&& authority.PendingProjectionMatches(recurrentQkvProgram));
	assert(authority.Complete(recurrentQkvProgram));
	assert(authority.GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex));
	DataflowProjectionProgram recurrentGateProgram{};
	assert(authority.BuildProjection(0,RecurrentGate,0,
		DataflowGraphRecurrentGateOutputSlot,recurrentGateProgram));
	assert(recurrentGateProgram.outputRows==DeltaInner);
	assert(authority.Begin(recurrentGateProgram));
	assert(authority.Complete(recurrentGateProgram));
	assert(authority.GraphWorkspaceReady(DataflowGraphRecurrentGateWorkspaceIndex));
	DataflowProjectionProgram recurrentAlphaProgram{};
	assert(authority.BuildProjection(0,Alpha,0,
		DataflowGraphRecurrentAlphaOutputSlot,recurrentAlphaProgram));
	assert(recurrentAlphaProgram.outputRows==DeltaHeads
		&& recurrentAlphaProgram.activeCores==2);
	unsigned alphaActive=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowCoreInvocation invocation{};
		assert(DataflowProgramBuilder::BindCore(recurrentAlphaProgram,core,invocation));
		alphaActive+=invocation.assignment.active;
	}
	assert(alphaActive==2);
	assert(DataflowProjectionMatchesGraphWorkspace(manifest,graphWorkspace,
		TestBankRoutes,recurrentAlphaProgram,nullptr,&graphIndex));
	assert(graphIndex==DataflowGraphRecurrentAlphaWorkspaceIndex);
	assert(authority.Begin(recurrentAlphaProgram));
	assert(authority.Complete(recurrentAlphaProgram));
	assert(authority.GraphWorkspaceReady(DataflowGraphRecurrentAlphaWorkspaceIndex));
	DataflowProjectionProgram recurrentBetaProgram{};
	assert(authority.BuildProjection(0,Beta,0,
		DataflowGraphRecurrentBetaOutputSlot,recurrentBetaProgram));
	assert(recurrentBetaProgram.outputRows==DeltaHeads
		&& recurrentBetaProgram.activeCores==2);
	assert(DataflowProjectionMatchesGraphWorkspace(manifest,graphWorkspace,
		TestBankRoutes,recurrentBetaProgram,nullptr,&graphIndex));
	assert(graphIndex==DataflowGraphRecurrentBetaWorkspaceIndex);
	assert(authority.Begin(recurrentBetaProgram));
	assert(authority.Complete(recurrentBetaProgram));
	assert(authority.GraphWorkspaceReady(DataflowGraphRecurrentBetaWorkspaceIndex));
	assert(!authority.BuildProjection(3,RecurrentQkv,0,
		DataflowGraphRecurrentQkvOutputSlot,recurrentQkvProgram));
	assert(authority.ResetRecurrentState());
	assert(authority.MarkActivationInitialized(0,Hidden));
	DataflowProjectionProgram denseKeyProgram{},denseValueProgram{},denseQueryProgram{};
	assert(DataflowProgramBuilder::Build(denseKey,manifest.activation[0],Hidden,
		graphWorkspace.space[DataflowGraphRecurrentGateWorkspaceIndex],
		TestBankRoutes,denseKeyProgram));
	assert(!authority.Authorize(denseKeyProgram));
	assert(DataflowProgramBuilder::Build(denseValue,manifest.activation[0],Hidden,
		graphWorkspace.space[DataflowGraphRecurrentGateWorkspaceIndex],
		TestBankRoutes,denseValueProgram));
	assert(!authority.Authorize(denseValueProgram));
	assert(authority.BuildDenseProjection(3,QueryAndGate,0,denseQueryProgram));
	assert(denseQueryProgram.outputRows==8192 && denseQueryProgram.activeCores==120);
	unsigned denseQueryActive=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		DataflowCoreInvocation invocation{};
		assert(DataflowProgramBuilder::BindCore(denseQueryProgram,core,invocation));
		denseQueryActive+=invocation.assignment.active;
	}
	assert(denseQueryActive==DataflowCoreCount);
	assert(authority.Begin(denseQueryProgram)
		&& authority.PendingProjectionMatches(denseQueryProgram));
	assert(authority.Complete(denseQueryProgram)
		&& authority.GraphWorkspaceReady(DataflowGraphRecurrentQkvWorkspaceIndex)
		&& authority.GraphWorkspaceLayer(DataflowGraphRecurrentQkvWorkspaceIndex)==3
		&& authority.GraphWorkspaceRole(DataflowGraphRecurrentQkvWorkspaceIndex)
			==QueryAndGate);
	assert(authority.BuildDenseProjection(3,Key,0,denseKeyProgram));
	assert(denseKeyProgram.outputRows==1024 && denseKeyProgram.activeCores==64);
	assert(authority.Begin(denseKeyProgram)
		&& authority.PendingProjectionMatches(denseKeyProgram));
	assert(authority.Complete(denseKeyProgram)
		&& authority.GraphWorkspaceReady(DataflowGraphRecurrentGateWorkspaceIndex)
		&& authority.GraphWorkspaceRole(DataflowGraphRecurrentGateWorkspaceIndex)==Key);
	// The Value projection is gated on a successful Key cache write, not only
	// on the Key matrix projection. Dense-attention sequencing covers that path.
	assert(!authority.BuildDenseProjection(3,Value,0,denseValueProgram));
	DataflowSiluProgram siluProgram{};
	assert(authority.BuildSilu(1,siluProgram) && siluProgram.Valid());
	assert(authority.BeginSilu(siluProgram));
	assert(authority.Pending() && !authority.GraphUpReady()
		&& !authority.ActivationReady(1)
		&& authority.PendingSiluMatches(siluProgram));
	assert(authority.CompleteSilu(siluProgram));
	assert(!authority.Pending() && authority.ActivationReady(1)
		&& authority.ActivationRows(1)==Ffn && !authority.GraphUpReady());
	DataflowResidualProgram residualProgram{};
	assert(!authority.BuildResidual(DataflowResidualAdd,1,residualProgram));
	assert(authority.BuildResidual(DataflowResidualSave,1,residualProgram));
	assert(authority.BeginResidual(residualProgram));
	assert(authority.Pending() && !authority.GraphResidualReady()
		&& authority.ActivationReady(1)
		&& authority.PendingResidualMatches(residualProgram));
	assert(authority.CompleteResidual(residualProgram));
	assert(!authority.Pending() && authority.GraphResidualReady()
		&& authority.ActivationReady(1));
	assert(authority.BuildResidual(DataflowResidualAdd,1,residualProgram));
	assert(authority.BeginResidual(residualProgram));
	assert(authority.Pending() && !authority.GraphResidualReady()
		&& !authority.ActivationReady(1));
	assert(authority.CompleteResidual(residualProgram));
	assert(!authority.Pending() && !authority.GraphResidualReady()
		&& authority.ActivationReady(1)
		&& authority.ActivationRows(1)==Hidden);
	assert(authority.BuildResidual(DataflowResidualSave,1,residualProgram));
	assert(authority.BeginResidual(residualProgram));
	assert(authority.AbortPendingProjection());
	assert(!authority.Pending() && !authority.GraphResidualReady()
		&& authority.ActivationReady(1)
		&& !authority.BuildResidual(DataflowResidualAdd,1,residualProgram));

	TensorSpace unregisteredWeight=weight;
	unregisteredWeight.place.parts[0].address+=4096;
	DataflowProjectionProgram unregisteredWeightProgram{};
	assert(BuildProgram(unregisteredWeight,input,Hidden,output,
		unregisteredWeightProgram));
	assert(unregisteredWeightProgram.Valid());
	assert(!DataflowProjectionMatchesLayout(manifest,TestBankRoutes,
		unregisteredWeightProgram));
	assert(!authority.Begin(unregisteredWeightProgram));
	DataflowProjectionAuthority incompleteAuthority;
	assert(incompleteAuthority.BindValidatedModel(manifest,graphWorkspace,TestBankRoutes));
	assert(incompleteAuthority.MarkActivationInitialized(0));
	assert(incompleteAuthority.Begin(program));
	assert(!incompleteAuthority.Complete(unregisteredWeightProgram));
	assert(incompleteAuthority.Pending()
		&& !incompleteAuthority.ActivationReady(1));

	RowSpace unregisteredInput=input;
	unregisteredInput.parts[0].address+=4096;
	DataflowProjectionProgram unregisteredInputProgram{};
	assert(BuildProgram(weight,unregisteredInput,Hidden,output,
		unregisteredInputProgram));
	assert(unregisteredInputProgram.Valid());
	assert(!DataflowProjectionMatchesLayout(manifest,TestBankRoutes,
		unregisteredInputProgram));

	DataflowProjectionProgram aliasedActivations{};
	assert(BuildProgram(weight,input,Hidden,input,aliasedActivations));
	assert(aliasedActivations.Valid());
	assert(!DataflowProjectionMatchesLayout(manifest,TestBankRoutes,
		aliasedActivations));

	unsigned unverifiedRoutes[2][Banks];
	memcpy(unverifiedRoutes,TestBankRoutes,sizeof(unverifiedRoutes));
	unverifiedRoutes[0][0]=787;
	DataflowProjectionProgram wrongRouteProgram{};
	assert(DataflowProgramBuilder::Build(weight,input,Hidden,output,
		unverifiedRoutes,wrongRouteProgram));
	assert(wrongRouteProgram.Valid());
	assert(!DataflowProjectionMatchesLayout(manifest,TestBankRoutes,
		wrongRouteProgram));
	assert(program.activeCores==120 && program.kernelRoles==DataflowAllRoles);
	assert(sizeof(DataflowCoreInvocation)<=DataflowProgramRegionBytes
		&& DataflowProgramAddress==0xaa000
		&& DataflowProgramRegionEnd==0xab000);
	assert(program.groups==Ffn/16);
	assert(program.circularBuffer[0].pageBytes==3392
		&& program.circularBuffer[0].pages==2
		&& program.circularBuffer[0].pageStride==DataflowWeightBufferPageStride
		&& DataflowWeightBufferAddress+DataflowWeightBufferPageStride==0x98000);
	assert(program.circularBuffer[1].pageBytes==576
		&& program.circularBuffer[1].pages==2);
	assert(program.circularBuffer[2].pageBytes==64
		&& program.circularBuffer[2].pages==2);
	assert(DataflowLocalCircularBuffersValid(program.circularBuffer));
	assert(!DataflowLocalRangesOverlap(DataflowWeightBufferAddress,3392,
		DataflowAffineCoefficientAddress,DataflowAffineMetadataBytes));
	assert(!DataflowLocalRangesOverlap(DataflowWeightBufferAddress
		+DataflowWeightBufferPageStride,3392,DataflowAffineMinimumAddress,
		DataflowAffineMetadataBytes));
	assert(!DataflowLocalRangesOverlap(DataflowActivationCacheAddress,
		DataflowActivationCacheBytes,DataflowQueueStateAddress,
		DataflowQueueStateBytes));
	DataflowCircularBuffer collidingScratch={0,DataflowQ5PanelScratchAddress,
		DataflowQ5PanelTileBytes,DataflowCircularBufferPages,
		DataflowQ5PanelTileBytes*DataflowCircularBufferPages,
		DataflowQ5PanelTileBytes};
	assert(!DataflowLocalCircularBufferValid(collidingScratch));
	DataflowCoreInvocation corePlan{};
	assert(DataflowProgramBuilder::BindCore(program,0,corePlan));
	DataflowQueueControl controls[DataflowCircularBufferCount]{};
	uint8_t local[DataflowLocalBufferEnd-DataflowWeightBufferAddress]{};
	DataflowQueueView queues[DataflowCircularBufferCount]{};
	assert(DataflowBindQueues(corePlan,controls,local,queues));
	for (unsigned i=0;i<DataflowCircularBufferCount;i++)
		assert(queues[i].local==local+(corePlan.circularBuffer[i].address-DataflowWeightBufferAddress));
	uint32_t ticket=0;
	uint8_t* page=nullptr;
	assert(DataflowQueueReserveBack(queues[0],ticket) && ticket==0);
	page=DataflowQueueWritePage(queues[0],ticket);
	assert(page==local && page[0]==0);
	page[0]=0x5a;
	assert(DataflowQueuePublishBack(queues[0],ticket));
	uint8_t* readPage=nullptr;
	assert(DataflowQueueWaitFront(queues[0],ticket,readPage)
		&& ticket==0 && readPage==page && readPage[0]==0x5a);
	assert(DataflowQueuePopFront(queues[0],ticket));
	assert(controls[0].produced==1 && controls[0].consumed==1);
	controls[0].produced=UINT32_MAX;
	controls[0].consumed=UINT32_MAX;
	assert(DataflowQueueReserveBack(queues[0],ticket) && ticket==UINT32_MAX);
	assert(DataflowQueuePublishBack(queues[0],ticket));
	assert(DataflowQueueWaitFront(queues[0],ticket,readPage)
		&& ticket==UINT32_MAX && readPage==local+DataflowWeightBufferPageStride);
	assert(DataflowQueuePopFront(queues[0],ticket));
	assert(controls[0].produced==0 && controls[0].consumed==0);
	DataflowQueueFail(queues[0],DataflowFailureCompute);
	DataflowQueueFail(queues[0],DataflowFailureCompute);
	DataflowQueueFail(queues[0],DataflowFailureReader);
	DataflowQueueFail(queues[0],DataflowFailureWriter);
	assert(DataflowLoadAcquire(controls[0].computeFailure)==DataflowFailureCompute);
	assert(DataflowLoadAcquire(controls[0].readerFailure)==DataflowFailureReader);
	assert(DataflowLoadAcquire(controls[0].writerFailure)==DataflowFailureWriter);
	assert(DataflowQueueFailure(controls[0])==DataflowFailureReader);
	assert(!DataflowQueueReserveBack(queues[0],ticket));
	static uint8_t activationCache[DataflowActivationCacheBytes];
	float* activation=reinterpret_cast<float*>(activationCache);
	PackTestOps packOps{};
	for (unsigned i=0;i<Hidden;i++) activation[i]=float(i);
	assert(DataflowPackActivationCache(corePlan,queues[1],activationCache,packOps));
	CheckPackedActivationPages(activationCache,14,Hidden);
	for (unsigned chunk=0;chunk<Hidden/256;chunk++) {
		assert(activationCache[chunk*DataflowInputPageBytes]==uint8_t(chunk));
		assert(activationCache[(chunk+1)*DataflowInputPageBytes-1]
			==uint8_t(chunk^0x5a));
	}
	assert(DataflowActivationChunk(14,9)==9);
	assert(DataflowActivationChunk(8,79)==9);
	assert(DataflowActivationChunk(13,9)==9);
	assert(DataflowActivationPackingType(8)==8
		&& DataflowActivationPackingType(12)==12
		&& DataflowActivationPackingType(13)==12
		&& DataflowActivationPackingType(14)==14);
	unsigned covered=0,splitOutputs=0;
	for (unsigned core=0;core<DataflowCoreCount;core++) {
		assert(program.core[core].active);
		DataflowCoreInvocation invocation{};
		assert(DataflowProgramBuilder::BindCore(program,core,invocation));
		assert(invocation.Valid() && invocation.core==core
			&& invocation.assignment.groups==program.core[core].groups);
		for (unsigned segment=0;segment<program.core[core].segmentCount;segment++) {
			const auto& weightSegment=program.core[core].weight[segment];
			const auto& outputSegment=program.core[core].output[segment];
			assert(weightSegment.firstGroup==covered);
			assert(outputSegment.firstRow==weightSegment.firstGroup*16);
			assert(outputSegment.rows==weightSegment.groups*16);
			covered+=weightSegment.groups;
			if (program.core[core].segmentCount>1) splitOutputs++;
		}
	}
	assert(covered==program.groups && splitOutputs>0);

	DataflowProjectionProgram corrupt=program;
	corrupt.core[0].weight[0].firstGroup++;
	assert(!corrupt.Valid());
	corrupt=program;
	corrupt.circularBuffer[0].pageStride=0x1000;
	assert(!corrupt.Valid());
	corrupt=program;
	corrupt.core[119].output[0].bank=(corrupt.core[119].output[0].bank+1)%Banks;
	assert(!corrupt.Valid());
	DataflowCoreInvocation corruptCore{};
	assert(DataflowProgramBuilder::BindCore(program,119,corruptCore));
	corruptCore.assignment.output[0].bank=(corruptCore.assignment.output[0].bank+1)%Banks;
	assert(!DataflowCoreHeaderValid(corruptCore));
	assert(DataflowProgramBuilder::BindCore(program,119,corruptCore));
	corruptCore.circularBuffer[0].pageStride=0x1000;
	assert(!DataflowCoreHeaderValid(corruptCore));

	TensorSpace q4Weight{};
	q4Weight.type=12;
	q4Weight.inputs=Ffn;
	Rows(q4Weight.place,Hidden,5184,0x08000000);
	DataflowProjectionProgram q4Program{};
	assert(BuildProgram(q4Weight,input,Ffn,output,q4Program));
	assert(q4Program.blocks==Ffn/256
		&& q4Program.circularBuffer[0].pageBytes==2304);
	DataflowCoreInvocation q4Core{};
	assert(DataflowProgramBuilder::BindCore(q4Program,0,q4Core));
	for (unsigned i=0;i<Ffn;i++) activation[i]=float(i);
	assert(DataflowPackActivationCache(q4Core,queues[1],activationCache,packOps));
	CheckPackedActivationPages(activationCache,12,Ffn);
	for (unsigned chunk=0;chunk<Ffn/256;chunk++)
		assert(activationCache[chunk*DataflowInputPageBytes]==uint8_t(chunk));

	TensorSpace q5Weight{};
	q5Weight.type=13;
	q5Weight.inputs=Hidden;
	Rows(q5Weight.place,Ffn,Hidden/256*(PioneerQwen35Projection::TileBytes(13)/16),
		0x08000000);
	DataflowProjectionProgram q5Program{};
	assert(BuildProgram(q5Weight,input,Hidden,output,q5Program));
	assert(q5Program.blocks==Hidden/256
		&& q5Program.circularBuffer[0].pageBytes==2816);
	DataflowCoreInvocation q5Core{};
	assert(DataflowProgramBuilder::BindCore(q5Program,0,q5Core));
	for (unsigned i=0;i<Hidden;i++) activation[i]=float(i);
	assert(DataflowPackActivationCache(q5Core,queues[1],activationCache,packOps));
	CheckPackedActivationPages(activationCache,13,Hidden);
	for (unsigned chunk=0;chunk<Hidden/256;chunk++)
		assert(activationCache[chunk*DataflowInputPageBytes]==uint8_t(chunk));

	struct ProjectionShape { unsigned type,inputs; };
	const ProjectionShape modelShapes[]={{8,2560},{12,2560},{13,2560},{14,2560},
		{12,4096},{13,4096},{12,9216},{14,9216}};
	for (const auto& shape:modelShapes) {
		TensorSpace shapeWeight{};
		shapeWeight.type=shape.type;
		shapeWeight.inputs=shape.inputs;
		const unsigned shapeBlocks=shape.inputs/(shape.type==8?32:256);
		const unsigned shapeRowBytes=shapeBlocks
			*(PioneerQwen35Projection::TileBytes(shape.type)/16);
		Rows(shapeWeight.place,Ffn,shapeRowBytes,0x08000000);
		DataflowProjectionProgram shapeProgram{};
		assert(BuildProgram(shapeWeight,input,shape.inputs,output,shapeProgram));
		assert(shapeProgram.blocks==shapeBlocks
			&& shapeProgram.circularBuffer[0].pageBytes
				==PioneerQwen35Projection::TileBytes(shape.type));
		for (unsigned core=0;core<DataflowCoreCount;core++) {
			DataflowCoreInvocation invocation{};
			assert(DataflowProgramBuilder::BindCore(shapeProgram,core,invocation));
			assert(invocation.Valid() && invocation.assignment.groups>0);
		}
	}

	TensorSpace q8Weight{};
	q8Weight.type=8;
	q8Weight.inputs=Hidden;
	Rows(q8Weight.place,Ffn,2880,0x08000000);
	DataflowProjectionProgram q8Program{};
	assert(BuildProgram(q8Weight,input,Hidden,output,q8Program));
	assert(q8Program.blocks==Hidden/32);
	DataflowCoreInvocation q8Core{};
	assert(DataflowProgramBuilder::BindCore(q8Program,0,q8Core));
	for (unsigned i=0;i<Hidden;i++) activation[i]=float(i);
	assert(DataflowPackActivationCache(q8Core,queues[1],activationCache,packOps));
	CheckPackedActivationPages(activationCache,8,Hidden);
	for (unsigned chunk=0;chunk<Hidden/256;chunk++)
		assert(activationCache[chunk*DataflowInputPageBytes]==uint8_t(chunk));

	TensorSpace invalidCoverage=weight;
	invalidCoverage.place.rows=Hidden/2;
	invalidCoverage.place.count=1;
	invalidCoverage.place.parts[0]={0,0x08000000,0,Hidden/2-16};
	for (unsigned i=1;i<Banks;i++) invalidCoverage.place.parts[i]={};
	DataflowProjectionProgram rejected{};
	assert(!BuildProgram(invalidCoverage,input,Hidden,output,rejected));
	unsigned duplicateRoutes[2][Banks];
	std::memcpy(duplicateRoutes,TestBankRoutes,sizeof(duplicateRoutes));
	duplicateRoutes[1][1]=duplicateRoutes[1][0];
	assert(!DataflowProgramBuilder::Build(weight,input,Hidden,output,
		duplicateRoutes,rejected));
	return 0;
}
